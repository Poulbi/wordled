@table(Name, KeySym) KeyboardButtons
{
    { Return     XK_Return }
    { Delete     XK_Delete }
    { BackSpace  XK_BackSpace }
    { Home       XK_KP_Home }
    { End        XK_KP_End }
    { PageUp     XK_KP_Prior }
    { PageDown   XK_KP_Next }
    { Control    XK_Control_L }
    { Shift      XK_Shift_L }
    { Alt        XK_Alt_L }
    { Insert     XK_Insert }
    { Escape     XK_Escape }
    { Up         XK_Up }
    { Down       XK_Down }
    { Left       XK_Left }
    { Right      XK_Right }
    { F1         XK_F1 }
    { F2         XK_F2 }
    { F3         XK_F3 }
    { F4         XK_F4 }
    { F5         XK_F5 }
    { F6         XK_F6 }
    { F7         XK_F7 }
    { F8         XK_F8 }
    { F9         XK_F9 }
    { F10        XK_F10 }
    { F11        XK_F11 }
    { F12        XK_F12 }
}


if(0) {}
@expand(KeyboardButtons k)
`if\(Symbol == $(k.KeySym)\)
{
 LinuxProcessKeyPress\(&KeyboardController->Keyboard.$(k.Name), IsDown\);
}`

@expand(KeyboardButtons k)
`game_button_state $(k.Name)`

typedef enum 
{
@expand(KeyboardButtons k) ` PlatformKeyboardButton_$(k.Name),`
 PlatformKeyboardButton_Count
} platform_keyboard_buttons;

@count(KeyboardButtons)

