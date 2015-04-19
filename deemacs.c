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
int64_t buf_sz;
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
  const char* description;
};

void refresh_all();

void refresh_status_bar( const char* extra_info );

// visual length
int64_t vlen( int64_t y )
{
 if ( y >= buf_sz || y < 0 )
   return 0;
 int64_t res = strlen(buf[y]);
 if ( res > 0 )
 {
   res -= buf[y][res-1]=='\n';
   if ( res > 0 )
   {
     // because windowz files are special snowflakes
     res -= buf[y][res-1]=='\r';
   }
 }
 return res;
}


// >>> options begin

// Use REALLOCF for reallocs with a smaller size than the previous alloc.
// Can be mapped either to "reallocf" or "realloc"
// where the first guarantues the new size and the latter may also keep the larger
// size being more efficient but potentially resulting in large unused memory blocks.
#define REALLOCF realloc

int option_show_newlines = 0;

/// >>>> functions begin

static void f_exit()
{
  exit(0);
}

void write_file();
void refresh_buffer( int64_t starting_from_line );
void add_special_buffer_message( int64_t y, int64_t x, const char* line );

static void f_save()
{
  char* tmp = malloc( strlen("saving ") + strlen( file_name ) + 1 );
  *tmp = 0;
  strcpy( tmp, "saving " );
  strcat( tmp, file_name );
  refresh_status_bar( tmp );
  write_file();
  strcpy( tmp, "saved " );
  strcat( tmp, file_name );
  refresh_status_bar( tmp );
  free(tmp);
}

int try_move_cursor_to_buf_pos( int64_t y, int64_t x );


static void f_isearch_forward();

static void f_forward_char() { if ( try_move_cursor_to_buf_pos( cur_buf_r(), cur_buf_c()+1 ) == 0 ) beep(); }
static void f_backward_char() { if ( try_move_cursor_to_buf_pos( cur_buf_r(), cur_buf_c()-1 ) == 0 ) beep(); }
static void f_next_line() { if ( try_move_cursor_to_buf_pos( cur_buf_r()+1, cur_buf_c() ) == 0 ) beep(); }
static void f_previous_line() { if ( try_move_cursor_to_buf_pos( cur_buf_r()-1, cur_buf_c() ) == 0 ) beep(); }
static void f_move_end_of_line() { if ( try_move_cursor_to_buf_pos( cur_buf_r(), vlen( cur_buf_r() ) ) == 0 ) beep(); }
static void f_move_beginning_of_line() { if ( try_move_cursor_to_buf_pos( cur_buf_r(), 0 ) == 0 ) beep(); }
static void f_recenter()
{
  if ( buf_sz <= nrows || cur_buf_r() < (nrows / 2) )
    return;
  int64_t old_cur_r = cur_r;
  cur_r = nrows / 2;
  buf_r = buf_r - cur_r + old_cur_r;
  refresh_all();
}
static void f_page_down()
{
  // emacs adds only nrows-2, we add one more. Emacs also only allows at least 3 rows for a buffer.
  if ( nrows <= 1 )
    ++buf_r;
  else
    buf_r += nrows - 1;
  if ( buf_r >= buf_sz )
    buf_r = buf_sz - 1;
  if ( cur_buf_r() >= buf_sz )
    cur_r = buf_sz - buf_r - 1;
  refresh_all();
}
/*static void f_page_up()
{
}*/

static void f_add_return();

static void f_backspace_function();
static void f_delete_function();

static void f_keyboard_quit() { refresh_all(); beep(); }

static void f_beginning_of_buffer()
{
  cur_c = cur_r = buf_r = buf_c = 0;
  refresh_all();
}

static void f_option_show_newlines()
{
  option_show_newlines = ! option_show_newlines;
  const char* msgon = "set show_newlines to on";
  const char* msgoff = "set show_newlines to off";
  refresh_all();
  refresh_status_bar( option_show_newlines ? msgon : msgoff );
}

static void f_revert_buffer();

static void f_show_keybindings();

static void f_kill_line();

/// <<<< functions end


struct Binding bindings[] =
{
// movement
  { 'n' | KBD_CTRL, KBD_NOKEY, f_next_line, "next line" },
  { 'p' | KBD_CTRL, KBD_NOKEY, f_previous_line, "previous line" },
  { 'f' | KBD_CTRL, KBD_NOKEY, f_forward_char, "one character forward" },
  { 'b' | KBD_CTRL, KBD_NOKEY, f_backward_char, "one character backward" },
  { KBD_DOWN, KBD_NOKEY, f_next_line, "next line" },
  { KBD_UP, KBD_NOKEY, f_previous_line, "previous line" },
  { KBD_RIGHT, KBD_NOKEY, f_forward_char, "one character forward" },
  { KBD_LEFT, KBD_NOKEY, f_backward_char, "one character backward" },
  { KBD_RET, KBD_NOKEY, f_add_return, "insert return" },
  { KBD_BS, KBD_NOKEY, f_backspace_function, "backspace" },
  { KBD_DEL, KBD_NOKEY, f_delete_function, "delete one character" },
  { 'd' | KBD_CTRL, KBD_NOKEY, f_delete_function, "delete one character" },

  { 'v' | KBD_CTRL, KBD_NOKEY, f_page_down, "move one page down" },

  { '<' | KBD_META, KBD_NOKEY, f_beginning_of_buffer, "move to beginning of buffer" },

  { 'l' | KBD_CTRL, KBD_NOKEY, f_recenter, "center cursor" },

  { 'a' | KBD_CTRL, KBD_NOKEY, f_move_beginning_of_line, "move to beginning of line" },
  { 'e' | KBD_CTRL, KBD_NOKEY, f_move_end_of_line, "move to end of line" },

  { 'o' | KBD_META, 'n', f_option_show_newlines, "option on/off: show newlines" },

  { 'u' | KBD_CTRL | KBD_META, KBD_NOKEY, f_revert_buffer, "revert buffer" }, //< this is bound to SUPER-u in emacs

  { 'x' | KBD_CTRL, 'c' | KBD_CTRL, f_exit, "exit" },
  { 'x' | KBD_CTRL, 's' | KBD_CTRL, f_save, "save buffer to file" },

  { 's' | KBD_CTRL, KBD_NOKEY, f_isearch_forward, "search forward" },

  { 'g' | KBD_CTRL, KBD_NOKEY, f_keyboard_quit, "exit command" },

  { 'k' | KBD_CTRL, KBD_NOKEY, f_kill_line, "delete until end of line" },

  { 'h' | KBD_CTRL, 'b', f_show_keybindings, "show keybindings" }, //< KBD_CTRL+h is often translated as backspace in terminal
  { '?' | KBD_META, KBD_NOKEY, f_show_keybindings, "show keybindings" }
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
  for ( int64_t i = 0; i < buf_sz; ++i )
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
  if ( buf_sz == buf_cap )
  {
    buf_cap = vec_next_size( buf_cap );
    buf = realloc( buf, buf_cap*sizeof(void*) );
    if ( ! buf ) err( EX_OSERR, "" );
  }
  // capacity is good enough
  if ( line_num + 1 < buf_sz )
  {
    // shift
    memmove( buf + line_num + 1, buf + line_num, (buf_sz - line_num)*sizeof(void*) );
  }
  ++buf_sz;
  buf[ line_num ] = s;
}
void append_to_buf( char* s )
{
  add_to_buf( s, buf_sz );
}
void remove_line_from_buf( int64_t line_num )
{
  free( buf[line_num] );
  if ( line_num + 1 < buf_sz )
  {
    memmove( buf + line_num, buf + line_num + 1, (buf_sz - line_num - 1)*sizeof(void*) );
  }
  --buf_sz;
}

void free_buffer()
{
  for ( int64_t i = 0; i < buf_sz; ++i )
    free( buf[i] );
  free( buf );
  buf_sz = 0;
  buf_cap = 0;
  buf_r = 0;
  buf_c = 0;
  cur_r = 0;
  cur_c = 0;
}

void open_file( bool create_if_not_exists );

static void f_revert_buffer()
{
  free_buffer();
  open_file( 0 );
  refresh_all();
}


// pos==1 => delete first char in line. pos==0 => delete newline from previous line
void remove_char_from_buf( int64_t line_num, int64_t pos )
{
  int64_t len = strlen( buf[line_num] );
  assert( pos <= len );
  if ( pos > 0 )
  {
    // ez
    memmove( buf[line_num] + pos - 1, buf[line_num] + pos, len-pos+1 );
    buf[line_num] = REALLOCF( buf[line_num], len-1+1 );
  }
  else
  {
    // collapse two rows
    if ( line_num == 0 )
      return;
    int64_t len2 = strlen( buf[line_num-1] );
    buf[line_num-1] = realloc( buf[line_num-1], len + len2 ); //< one newline will be removed
    memcpy( buf[line_num-1] + len2 - 1, buf[line_num], len + 1 );
    remove_line_from_buf( line_num );
  }
}

static void f_kill_line()
{
  int64_t c = cur_buf_c();
  int64_t r = cur_buf_r();
  int64_t len = strlen( buf[r] );

  if ( c+1 == len )
  {
    f_delete_function();
    return;
  }
  buf[r][c]='\n';
  buf[r][c+1]=0;
  refresh_all();
}

static void f_show_keybindings()
{
  for ( int i = 0; i < sizeof(bindings) / sizeof(bindings[0]); ++i )
  {
    struct Binding* tmp = &bindings[i];
    int32_t first_key = tmp->first;
    int32_t second_key = tmp->second;

    char to_print[256];
    to_print[0] = 0;
    char* tmp1 = deemacs_key_to_str_representation( first_key );
    strcat( to_print, tmp1 );
    free(tmp1);

    if ( second_key != KBD_NOKEY )
    {
      char* tmp = deemacs_key_to_str_representation( second_key );
      strcat( to_print, " ");
      strcat( to_print, tmp);
      free( tmp );
    }

    int left_col_start = 24;
    for (int i=strlen(to_print); i<left_col_start; ++i )
    {
      to_print[i]=' ';
      to_print[i+1]=0;
    }

    strcat( to_print, tmp->description );

    add_special_buffer_message(i, 0, to_print);
  }

}

void f_backspace_function()
{
  int64_t c = cur_buf_c();
  int64_t r = cur_buf_r();
  if ( c != 0 )
  {
    remove_char_from_buf( cur_buf_r(), cur_buf_c() );
    --cur_c;
  }
  else if ( r != 0 )
  {
    int64_t pos = strlen( buf[r-1] );
    remove_char_from_buf( cur_buf_r(), cur_buf_c() );
    --cur_r;
    cur_c = pos - 1; //< -1 cause of newline
  }
  else
  {
    beep();
    return;
  }
  refresh_all();
}

void f_delete_function()
{
  int64_t c = cur_buf_c();
  int64_t r = cur_buf_r();
  int64_t len = strlen( buf[r] );
  if ( c < len-1 )
  {
    ++cur_c;
    f_backspace_function();
  }
  else if ( r + 1 < buf_sz )
  {
    cur_c = 0;
    ++cur_r;
    f_backspace_function();
  }
  else
  {
    beep();
    return;
  }
  refresh_all();
}

void add_char_to_buf( char c, int64_t line_num, int64_t pos )
{
  int64_t len = strlen( buf[line_num] );
  buf[line_num] = realloc( buf[line_num], len + 1 + 1 );
  memmove( buf[line_num] + pos +1, buf[line_num] + pos, len-pos+1 );
  buf[line_num][pos]=c;
}

void add_newline_to_buf( int64_t line_num, int64_t pos )
{
  char* line = buf[line_num];
  int64_t len = strlen( line );
  char* second = malloc( len - pos + 1 );
  memcpy( second, line+pos, len - pos + 1 );
  char* first = REALLOCF( buf[line_num], pos + 2 );
  *(first+pos) = '\n';
  *(first+pos+1) = 0;
  buf[line_num] = first;
  add_to_buf( first, line_num );
  buf[line_num+1] = second;
}


int try_move_cursor_to_buf_pos( int64_t y, int64_t x )
{
  if ( y < 0 || y >= buf_sz || x < 0 )
    return 0;
  int64_t len = vlen( y );
  if ( x > len )
    return 0;

  // Where should cursor go on the display? does it still fit into display?

  int64_t ydiff = y - buf_r;
  int64_t xdiff = x - buf_c;
  if ( ydiff >= 0 && ydiff < nrows && xdiff >= 0 && xdiff < ncols )
  {
    // finished - only move required
    cur_r = ydiff;
    cur_c = xdiff;
    move( cur_r, cur_c );
    return 1;
  }

  // cursor does not fit on display - we need to change buf_r/buf_c

  if ( ydiff < 0 )
  {
    // lines fit all into screen
    if ( nrows >= buf_sz )
    {
      buf_r = 0;
      cur_r = 0;
    }
    else
    {
      buf_r = y - nrows/2;
      if ( buf_r < 0 )
        buf_r = 0;
      cur_r = y - buf_r;
    }
  }
  else if ( ydiff >= nrows )
  {
    buf_r = y - nrows/2;
    if ( buf_r < 0 )
      buf_r = 0;
    cur_r = y - buf_r;
  }

  refresh_all();

/*
  if ( xdiff < 0 )
  {
    xdiff = 0;
  }
  else if ( xdiff > 
*/
  return 1;
}

void open_file( bool create_if_not_exists )
{
  f = fopen( file_name, "r+" );
  // try create
  if ( ! f && create_if_not_exists )
  {
    errno = 0;
    f = fopen( file_name, "w+" );
  }
  if ( ! f ) err( EX_NOINPUT, "%s", file_name );
  buf = realloc( buf, sizeof(char*) * INIT_VEC_SIZE );
  buf_sz = 0;
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
  tmp_ptr = malloc(1);
  tmp_ptr[0] = 0;
  append_to_buf( tmp_ptr );
  if ( fclose( f ) != 0 ) err( EX_IOERR, "%s", file_name );
}

void debug_print_buf()
{
  for ( int i =0; i<buf_sz; ++i )
    fwrite( buf[i], 1, strlen(buf[i]), stdout );
}

void editor();

int main( int argn, char** argv )
{
  setlocale(LC_ALL, "");
  err_set_exit( cleanup );
  bool create_if_not_exists = 0;

  if ( argn == 3 && strcmp( argv[1], "-c" ) == 0 )
  {
    create_if_not_exists = 1;
    argn = 2;
    argv[1] = argv[2];
  }
  if ( argn != 2 )
  {
    errx( EX_USAGE, "%s","usage: deemacs [-c] [file]" );
  }
  file_name = argv[1];

  open_file( create_if_not_exists );

  atexit( cleanup_at_exit );

  editor();

  endwin();
  cleanup(0);
  return 0;
}

void add_special_buffer_message( int64_t y, int64_t x, const char* line )
{
  if ( has_color ) attron(COLOR_PAIR(2));
  attron(A_STANDOUT);
  if ( y <= nrows )
    mvaddstr( y, x, line );
  attroff(A_STANDOUT);
  if ( has_color ) attroff(COLOR_PAIR(2));
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

  printw( "    %d%%  (%d/%d,%d/%d)", (buf_r)*100/buf_sz, cur_buf_r()+1, buf_sz, cur_buf_c(), strlen(buf[cur_buf_r()]) );

  clrtoeol();

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
  for ( ; i < nrows && (i+buf_r) < buf_sz; ++i )
  {
    if ( i + buf_r > buf_sz )
      break;
    int64_t slen = strlen(buf[buf_r+i]);
    if ( buf_c > slen ) continue;
    if ( slen - buf_c > ncols )
    {
      char tmp = buf[buf_r+i][buf_c+ncols];
      buf[buf_r+i][buf_c+ncols] = 0;
      mvaddstr( i, 0, buf[buf_r+i] );
      buf[buf_r+i][buf_c+ncols] = tmp;
    }
    else
    {
      mvaddstr( i, 0, buf[buf_r+i] );
      if ( option_show_newlines )
      {
        if ( has_color ) attron(COLOR_PAIR(3));
        mvaddstr( i, slen-1, " " );
        if ( has_color ) attroff(COLOR_PAIR(3));
      }
    }
    clrtoeol();
  }
  for ( ; i < nrows; ++i )
  {
    move( i, 0 );
    clrtoeol();
  }
  move( cur_r, cur_c );
}

void refresh_all()
{
  refresh_buffer( 0 );
  refresh_status_bar( 0 );
  move( cur_r, cur_c );
  refresh();
}

void init_colors()
{
  has_color = has_colors();
  if ( ! has_color )
    return;
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  // 8 is grey
  init_pair(3, COLOR_WHITE, 8);
}


void key_is_undefined_action( int32_t first, int32_t second )
{
  char to_print[64];
  to_print[0] = 0;
  char* tmp1 = deemacs_key_to_str_representation( first );
  strcat( to_print, tmp1 );
  free(tmp1);

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
}

void f_add_char( int32_t c, int32_t no_key )
{
  assert( no_key == KBD_NOKEY );
  add_char_to_buf( c, cur_buf_r(), cur_buf_c() );
  ++cur_c;
  refresh_all();
}


void f_add_return()
{
  add_newline_to_buf( cur_buf_r(), cur_buf_c() );
  ++cur_r;
  cur_c = 0;
  refresh_all();
}

bool handle_input()
{
  int32_t first_key = deemacs_next_key();

  if ( first_key <= 255 && ( isgraph( first_key ) || first_key == ' ' ) )
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
//  printw( " %d", second_key );

  // special case: CTRL-G: break everything
  if ( second_key == (KBD_CTRL | 'g') )
  {
    f_keyboard_quit();
    return true;
  }

  char* keystr2 = deemacs_key_to_str_representation( second_key );
//  addstr( keystr2 );
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

static bool find_next_in_buffer( int64_t r, int64_t c, int64_t* r2, int64_t* c2, const char* needle )
{
  // contains upper
  bool has_upper = false;
  for ( int i = 0; i < strlen(needle); ++i )
    if ( isupper( needle[i] ) )
      has_upper = true;

  char* match = has_upper ? strcasestr( buf[r]+c, needle ) : strstr( buf[r]+c, needle );
  if ( match != 0 )
  {
    *r2=r;
    *c2=match-buf[r];
    return true;
  }

  for ( int64_t cr=r+1;cr<buf_sz;++cr )
  {
    char* match = has_upper ? strcasestr( buf[cr], needle ) : strstr( buf[cr], needle );
    if ( match != 0 )
    {
      *r2=cr;
      *c2=match-buf[cr];
      return true;
    }
  }

  return false;
}


static void isearch( bool backward /* todo(dees): backword is not working, fix */ )
{
  int64_t c = cur_buf_c();
  int64_t r = cur_buf_r();

  char* needle = malloc(32);
  int needle_cap = 32;
  needle[0] = 0;

  int match_next = 0;

  const char status_msg_failing[] = "Failing search: ";
  const char status_msg_wrapped[] = "Wrapped search: ";
  const char status_msg_prefix[] = "search: ";
  // keep up-to date with the 3 msgs
  int status_prefix_max_len = strlen( status_msg_wrapped );

  refresh_status_bar( status_msg_prefix );

  while ( 1 )
  {
    int32_t first_key = deemacs_next_key();

    // out
    if ( first_key == (KBD_CTRL | 'g') )
    {
      if ( cur_buf_c() != c || cur_buf_r() != r )
        try_move_cursor_to_buf_pos( r, c );
      free(needle);
      return;
    }
    // delete one char
    else if ( first_key == KBD_BS )
    {
      if ( match_next > 0 )
        --match_next;
      else if ( strlen( needle ) > 0 )
        needle[ strlen(needle) - 1 ] = 0;
      else
        beep();
    }
    // next match
    else if ( first_key == ('s' | KBD_CTRL) )
      ++match_next;
    // finish search
    else if ( first_key == KBD_RET )
    {
      free(needle);
      return;
    }
    // add character to search pattern
    else if ( first_key <= 255 && ( isgraph( first_key ) || first_key == ' ' ) )
    {
      int nlen = strlen(needle);
      if ( nlen+1 >= needle_cap )
      {
        needle = REALLOCF( needle, needle_cap*2 );
        needle_cap *= 2;
      }
      needle[ nlen ] = first_key;
      needle[ nlen + 1 ] = 0;
    }
    else
    {
      key_is_undefined_action( first_key, KBD_NOKEY );
      continue;
    }

    int64_t cspos = c;
    int64_t crpos = r;
    bool has_wrapped = 0;
    bool has_matched = 0;
    for ( int nmatch = 0; nmatch <= match_next; ++nmatch )
    {
      int64_t rmatch;
      int64_t cmatch;
      bool is_found = find_next_in_buffer( crpos, cspos, &rmatch, &cmatch, needle );
      if ( ! is_found && has_wrapped && crpos == 0 && cspos == 0 )
      {
        beep();
      }
      else if ( ! is_found )
      {
        has_wrapped = 1;
        cspos = 0;
        crpos = 0;
      }
      else if ( is_found && nmatch == match_next ) //< bingo
      {
        try_move_cursor_to_buf_pos( rmatch, cmatch + strlen(needle) );
        cspos = cmatch;
        crpos = rmatch;
        has_matched = 1;
      }
      else
      {
        assert( is_found );
        assert( nmatch != match_next );
        cspos = cmatch+1;
        crpos = rmatch;
      }
    }

    char* status_msg = malloc(strlen(needle)+status_prefix_max_len+1);
    status_msg[0] = 0;
    if ( ! has_matched )
      strcat( status_msg, status_msg_failing );
    else if ( has_wrapped )
      strcat( status_msg, status_msg_wrapped );
    else
      strcat( status_msg, status_msg_prefix );
    strcat( status_msg, needle );
    refresh_status_bar( status_msg );
    free(status_msg);

    // highlite match
    if (has_matched)
    {
      add_special_buffer_message(crpos-buf_r,cspos-buf_c,needle);
    }

  }

}


static void f_isearch_forward()
{
  isearch( false );
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
