#include "input.h"

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int32_t codetokey (int32_t c)
{
  switch (c)
    {
    case '\0':			/* C-@ */
      return KBD_CTRL | '@';
    case '\1':
    case '\2':
    case '\3':
    case '\4':
    case '\5':
    case '\6':
    case '\7':
    case '\10':
    case '\12':
    case '\13':
    case '\14':
    case '\16':
    case '\17':
    case '\20':
    case '\21':
    case '\22':
    case '\23':
    case '\24':
    case '\25':
    case '\26':
    case '\27':
    case '\30':
    case '\31':
    case '\32':			/* C-a ... C-z */
      return KBD_CTRL | ('a' + c - 1);
    case '\11':
      return KBD_TAB;
    case '\15':
      return KBD_RET;
    case '\37':
      return KBD_CTRL | '_';
#ifdef KEY_SUSPEND
    case KEY_SUSPEND:		/* C-z */
      return KBD_CTRL | 'z';
#endif
    case '\33':			/* META */
      return KBD_META;
    case KEY_PPAGE:		/* PGUP */
      return KBD_PGUP;
    case KEY_NPAGE:		/* PGDN */
      return KBD_PGDN;
    case KEY_HOME:
      return KBD_HOME;
    case KEY_END:
      return KBD_END;
    case KEY_DC:		/* DEL */
      return KBD_DEL;
    case KEY_BACKSPACE:		/* Backspace or Ctrl-H */
      return KBD_BS;
    case 0177:			/* BS */
      return KBD_BS;
    case KEY_IC:		/* INSERT */
      return KBD_INS;
    case KEY_LEFT:
      return KBD_LEFT;
    case KEY_RIGHT:
      return KBD_RIGHT;
    case KEY_UP:
      return KBD_UP;
    case KEY_DOWN:
      return KBD_DOWN;
    case KEY_F (1):
      return KBD_F1;
    case KEY_F (2):
      return KBD_F2;
    case KEY_F (3):
      return KBD_F3;
    case KEY_F (4):
      return KBD_F4;
    case KEY_F (5):
      return KBD_F5;
    case KEY_F (6):
      return KBD_F6;
    case KEY_F (7):
      return KBD_F7;
    case KEY_F (8):
      return KBD_F8;
    case KEY_F (9):
      return KBD_F9;
    case KEY_F (10):
      return KBD_F10;
    case KEY_F (11):
      return KBD_F11;
    case KEY_F (12):
      return KBD_F12;
    default:
      if (c > 0xff || c < 0)
        return KBD_NOKEY;	/* ERR (no key) or undefined behaviour. */
      return c;
    }
}

char* deemacs_key_to_str_representation( int32_t key )
{
  // C-M-<backspace> is longest string with 11+2+2+1=16 chars.
  char* res = malloc( 16 );
  *res = 0;

  if (key & KBD_CTRL)
    strcat (res, "C-");
  if (key & KBD_META)
    strcat (res, "M-");
  key &= ~(KBD_CTRL | KBD_META);

  switch (key)
    {
    case KBD_PGUP:
      strcat(res, "<prior>");
      break;
    case KBD_PGDN:
      strcat(res, "<next>");
      break;
    case KBD_HOME:
      strcat(res, "<home>");
      break;
    case KBD_END:
      strcat(res, "<end>");
      break;
    case KBD_DEL:
      strcat(res, "<delete>");
      break;
    case KBD_BS:
      strcat(res, "<backspace>");
      break;
    case KBD_INS:
      strcat(res, "<insert>");
      break;
    case KBD_LEFT:
      strcat(res, "<left>");
      break;
    case KBD_RIGHT:
      strcat(res, "<right>");
      break;
    case KBD_UP:
      strcat(res, "<up>");
      break;
    case KBD_DOWN:
      strcat(res, "<down>");
      break;
    case KBD_RET:
      strcat(res, "<RET>");
      break;
    case KBD_TAB:
      strcat(res, "<TAB>");
      break;
    case KBD_F1:
      strcat(res, "<f1>");
      break;
    case KBD_F2:
      strcat(res, "<f2>");
      break;
    case KBD_F3:
      strcat(res, "<f3>");
      break;
    case KBD_F4:
      strcat(res, "<f4>");
      break;
    case KBD_F5:
      strcat(res, "<f5>");
      break;
    case KBD_F6:
      strcat(res, "<f6>");
      break;
    case KBD_F7:
      strcat(res, "<f7>");
      break;
    case KBD_F8:
      strcat(res, "<f8>");
      break;
    case KBD_F9:
      strcat(res, "<f9>");
      break;
    case KBD_F10:
      strcat(res, "<f10>");
      break;
    case KBD_F11:
      strcat(res, "<f11>");
      break;
    case KBD_F12:
      strcat(res, "<f12>");
      break;
    case ' ':
      strcat(res, "SPC");
      break;
    default:
      if (key <= 0xff && isgraph (key))
      {
        int l = strlen( res );
        res[l] = key;
        res[l+1] = 0;
      }
      else
        sprintf( res, "<%x>", (unsigned) key );
    }

  return res;
}

int32_t deemacs_next_key( void )
{
  int32_t key = codetokey( getch() );
  while ( key == KBD_META )
  {
    key = codetokey( getch() ) | KBD_META ;
  }
  return key;
}
