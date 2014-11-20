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

// file informations
FILE* f;
const  char* file_name;

bool has_color;

// buffer content
char** buf;
int64_t buf_size;
int64_t buf_cap;

// buffer position top left
int buf_r, buf_c;

// cursor position in buffer
int cur_r,cur_c;

// window informations
int nrows; //< nrows is minus 1 than actual nrows - this is used for the status bar
int ncols;

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
  buf[line_num] = realloc( buf[line_num], len );
  memmove( buf[line_num] + pos +1, buf[line_num] + pos, len-pos+1 );
}
void remove_char_from_buf( int64_t line_num, int64_t pos )
{
  int64_t len = strlen( buf[line_num] );
  memmove( buf[line_num]+pos, buf[line_num]+pos+1, (len - pos - 1 + 1 ) );
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

  editor();

  endwin();
  cleanup(0);
  return 0;
}

void refresh_status_bar()
{
  if ( nrows < 0 )
    return;
  if ( has_color ) attron(COLOR_PAIR(1));
  attron(A_BOLD);
  mvaddstr( nrows, 1, file_name );
  attroff(A_BOLD);
  if ( has_color ) attroff(COLOR_PAIR(1));

/*  addstr( "    (" );
  attron(A_BOLD);
  printw( "%d", cur_r+1 );
  attroff(A_BOLD);
  printw( "/%d,", buf_size );
  attron(A_BOLD);
  printw( "%d", cur_c );
  attroff(A_BOLD);
  printw( "/%d)", strlen(buf[cur_r]) );*/
  printw( "    %d%%  (%d/%d,%d/%d)", (buf_r)*100/buf_size, cur_r+1, buf_size, cur_c, strlen(buf[cur_r]) );
  clrtoeol();
  move( cur_r, cur_c );
}

void refresh_all()
{
  clear();
  for ( int64_t i = 0; i < nrows && i < buf_size; ++i )
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
  refresh_status_bar();
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
}


bool handle_input( int input )
{
  switch ( input )
  {
  case KEY_UP:
    --cur_r;
    move(cur_r,cur_c);
    refresh_status_bar();
    return true;
  case KEY_DOWN:
    ++cur_r;
    move(cur_r,cur_c);
    refresh_status_bar();
    return true;
  case KEY_RIGHT:
    ++cur_c;
    move(cur_r,cur_c);
    refresh_status_bar();
    return true;
  case KEY_LEFT:
    --cur_c;
    move(cur_r,cur_c);
    refresh_status_bar();
    return true;
  case 'q':
    return false;
  default:
    addch( input );
    return true;
  }
}

void editor()
{
  WINDOW* wnd = initscr();
  cbreak();
  noecho();
  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);

  init_colors();

  getmaxyx(wnd,nrows,ncols);
  --nrows;

  refresh_all();
  while ( handle_input( getch() ) );
}
