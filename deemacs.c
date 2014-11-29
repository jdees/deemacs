#include <stdio.h>
#include <curses.h>
#include <locale.h>
#include <err.h>
#include <assert.h>
#include <stdint.h>
#include <sysexits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "input.h"

// file informations
FILE* f;
const  char* file_name;

bool has_color;

// buffer content
char** buf;
int64_t buf_size;
int64_t buf_cap;

// buffer position top left
int64_t buf_r, buf_c;

// cursor position in editor buffer
int cur_r,cur_c;

// cursor position in buffer content
int64_t cur_buf_r() { return buf_r + cur_r; }
int64_t cur_buf_c() { return buf_c + cur_c; }

// window informations
int nrows; //< nrows is minus 1 than actual nrows - this is used for the status bar
int ncols;

// functions

typedef void (*function_t) (int32_t first, int32_t second);

struct Binding
{
  int first;
  int second;
  function_t func;
};

void f_exit()
{
  exit(0);
}

struct Binding bindings[] =
{
  { 'x' | KBD_CTRL, 'c' | KBD_CTRL, f_exit },
}
;

void cleanup_at_exit()
{
  endwin();
}

void cleanup( int eval )
{
  endwin();
}

void debug_print_buf();

const int64_t INIT_VEC_SIZE = 16;
int64_t vec_next_size( int64_t v )
{
  return v*2 < INIT_VEC_SIZE ? INIT_VEC_SIZE : v*2;
}

void write_file()
{
  f = fopen( file_name, "w+" );
  if ( ! f ) err( EX_IOERR, "%s", file_name );
  for ( int64_t i = 0; i < buf_size; ++i )
  {
    int64_t num_chars = strlen(buf[i]);
    if ( fwrite( buf[i], 1, num_chars, f ) < num_chars )
      err( EX_IOERR, "%s", file_name );
  }
  if ( fclose( f ) != 0 ) err( EX_IOERR, "%s", file_name );
}

//// buffer modification functions
void add_to_buf( char* s, int64_t line_num )
{
  if ( buf_size == buf_cap )
  {
    buf_cap = vec_next_size( buf_cap );
    buf = realloc( buf, buf_cap*sizeof(void*) );
    if ( ! buf ) err( EX_OSERR, "" );
  }
  // capacity is good enough
  if ( line_num + 1 < buf_size )
  {
    // shift
    memmove( buf + line_num + 1, buf + line_num, (buf_size - line_num)*sizeof(void*) );
  }
  ++buf_size;
  buf[ line_num ] = s;
}
void append_to_buf( char* s )
{
  add_to_buf( s, buf_size );
}
void remove_line_from_buf( int64_t line_num )
{
  free( buf[line_num] );
  if ( line_num + 1 < buf_size )
  {
    memmove( buf + line_num, buf + line_num + 1, (buf_size - line_num - 1)*sizeof(void*) );
  }
  --buf_size;
}
void add_char_to_buf( char c, int64_t line_num, int64_t pos )
{
  int64_t len = strlen( buf[line_num] );
  buf[line_num] = realloc( buf[line_num], len + 1 + 1 );
  memmove( buf[line_num] + pos +1, buf[line_num] + pos, len-pos+1 );
  buf[line_num][pos]=c;
}
void remove_char_from_buf( int64_t line_num, int64_t pos )
{
  int64_t len = strlen( buf[line_num] );
  memmove( buf[line_num]+pos, buf[line_num]+pos+1, (len - pos - 1 + 1 ) );
}

int try_move_cursor_to_buf_pos( int64_t y, int64_t x )
{
/*  if ( y < 0 || y >= buf_size || x < 0 )
    return 0;
  int64_t len = strlen( buf[ y ] );
  if ( x >= len )
    return 0;

  buf_r + cur_r

  move( cur_r, cur_c );
  
  buf_r = y;
  buf_c = x;*/
  return 0;
}

void open_file()
{
  f = fopen( file_name, "r+" );
  if ( ! f ) err( EX_NOINPUT, "%s", file_name );
  buf = realloc( buf, sizeof(char*) * INIT_VEC_SIZE );
  buf_size = 0;
  buf_cap = INIT_VEC_SIZE;
  char * tmp_ptr = 0;
  size_t lcap = 0;
  while ( 1 )
  {
    if ( getline( &tmp_ptr, &lcap, f ) == -1 )
    {
      if ( errno ) err( EX_IOERR, "%s", file_name );
      else break;
    }
    append_to_buf( tmp_ptr );
    tmp_ptr = 0;
  }
  if ( fclose( f ) != 0 ) err( EX_IOERR, "%s", file_name );
}

void debug_print_buf()
{
  for ( int i =0; i<buf_size; ++i )
    fwrite( buf[i], 1, strlen(buf[i]), stdout );
}

void editor();

int main( int argn, char** argv )
{
  setlocale(LC_ALL, "");
  err_set_exit( cleanup );
  if ( argn != 2 )
  {
    errx( EX_USAGE, "%s","usage: deemacs [file]" );
  }
  file_name = argv[1];

  open_file();

  atexit( cleanup_at_exit );

  editor();

  endwin();
  cleanup(0);
  return 0;
}

void refresh_status_bar( const char* extra_info )
{
  if ( nrows < 0 )
    return;
  if ( has_color ) attron(COLOR_PAIR(1));
  attron(A_BOLD);
  mvaddstr( nrows, 1, file_name );
  attroff(A_BOLD);
  if ( has_color ) attroff(COLOR_PAIR(1));

  printw( "    %d%%  (%d/%d,%d/%d)", (buf_r)*100/buf_size, cur_buf_r()+1, buf_size, cur_buf_c(), strlen(buf[cur_buf_r()]) );

  if ( extra_info && *extra_info != 0 )
  {
    int elen = strlen( extra_info );
    int xpos = ncols - elen - 1;
    if ( xpos < 0 ) xpos = 0;
    if ( has_color ) attron(COLOR_PAIR(2));
    attron(A_STANDOUT);
    mvaddstr( nrows, xpos, extra_info );
    attroff(A_STANDOUT);
    if ( has_color ) attroff(COLOR_PAIR(2));
  }
  
  clrtoeol();
  move( cur_r, cur_c );
}

void refresh_buffer( int64_t starting_from_line )
{
  int64_t i = starting_from_line;
  for ( ; i < nrows && i < buf_size; ++i )
  {
    if ( i + buf_r > buf_size )
      break;
    int64_t slen = strlen(buf[i]);
    if ( buf_c > slen ) continue;
    if ( slen - buf_c > ncols )
    {
      char tmp = buf[i][buf_c+ncols];
      buf[i][buf_c+ncols] = 0;
      mvaddstr( i, 0, buf[i] );
      buf[i][buf_c+ncols] = tmp;
    }
    else
      mvaddstr( i, 0, buf[i] );
    clrtoeol();
  }
  for ( ; i < nrows; ++i )
  {
    move( i, 0 );
    clrtoeol();
  }
}

void refresh_all()
{
  refresh_buffer( 0 );
  refresh_status_bar( 0 );
  move( cur_r, cur_c );
  // refresh(); not required?
}

void init_colors()
{
  has_color = has_colors();
  if ( ! has_color )
    return;
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
}


void key_is_undefined_action( int32_t first, int32_t second )
{
  char* to_print = deemacs_key_to_str_representation( first );

  if ( second != KBD_NOKEY )
  {
    char* tmp = deemacs_key_to_str_representation( second );
    strcat( to_print, " ");
    strcat( to_print, tmp);
    free( tmp );
  }

  strcat( to_print, " is undefined" );

  refresh_status_bar( to_print );
  
  beep();
  free( to_print );
}

void f_add_char( int32_t c, int32_t no_key )
{
  assert( no_key == KBD_NOKEY );
  add_char_to_buf( c, cur_buf_r(), cur_buf_c() );
  ++cur_c;
  refresh_all();
}

bool handle_input()
{
  int32_t first_key = deemacs_next_key();

  if ( first_key <= 255 && isalnum( first_key ) )
  {
    f_add_char( first_key, KBD_NOKEY );
    return true;
  }

  bool prefix_exists = 0;

  for ( int i = 0; i < sizeof(bindings) / sizeof(bindings[0]); ++i )
  {
    struct Binding* tmp = &bindings[i];
    if ( tmp->first == first_key )
    {
      prefix_exists = 1;
      if ( tmp->second == KBD_NOKEY )
      {
        // match
        tmp->func( first_key, KBD_NOKEY );
        return true;
      }
    }
  }

  if ( ! prefix_exists )
  {
    key_is_undefined_action( first_key, KBD_NOKEY );
    return true;
  }

  int32_t second_key = deemacs_next_key();
  printw( " %d", second_key );

  char* keystr2 = deemacs_key_to_str_representation( second_key );
  addstr( keystr2 );
  free( keystr2 );

  for ( int i = 0; i < sizeof(bindings) / sizeof(bindings[0]); ++i )
  {
    struct Binding* tmp = &bindings[i];
    if ( tmp->first == first_key && tmp->second == second_key )
    {
      tmp->func( first_key, KBD_NOKEY );
      return true;
    }
  }

  key_is_undefined_action( first_key, second_key );
  return true;
  
  
}

void editor()
{
  WINDOW* wnd = initscr();
  raw();
  noecho();
  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);

  init_colors();

  getmaxyx(wnd,nrows,ncols);
  --nrows;

  refresh_all();
  while ( handle_input() );
}
