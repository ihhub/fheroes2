#include "radio_buttons_group.h"

RadioButtonsGroup::RadioButtonsGroup( const Point & pos, uint amount, uint vSpacingStep )
{
    Point pt;

    if ( amount == 0 ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "passed amount of buttons for RadioButtonsGroup object is zero" );
        return;
    }

    for ( int i = 0; i < amount; ++i ) {
        buttons.push_back( new Button( pos.x, pos.y + ( i == 0 ) ? 0 : buttons[i - 1]->h * i + vSpacingStep * i, ICN::NGEXTRA, 66, 67 ) );
    }

    SetActiveButton( uint( 0 ) );
}

RadioButtonsGroup::~RadioButtonsGroup()
{
    for ( std::vector<Button *>::iterator iter = buttons.begin(); iter != buttons.end(); ++iter) {
        delete *iter;
    }
}

void RadioButtonsGroup::Draw( void )
{
    for ( std::vector<Button *>::iterator iter = buttons.begin(); iter != buttons.end(); ++iter) {
        ( *iter )->Draw();
    }
}

int RadioButtonsGroup::QueueEventProcessing( void )
{
    LocalEvent & le = LocalEvent::Get();

    uint iterableID = 0;
    for ( std::vector<Button *>::iterator iter = buttons.begin(); iter != buttons.end(); ++iter, ++iterableID) {
        if ( *iter && ( *iter )->isEnable() && le.MousePressLeft( **iter ) )
            SetActiveButton( iterableID );
    }

    return Dialog::ZERO;
}

void RadioButtonsGroup::SetActiveButton( uint activeID )
{
    uint iterableID = 0;
    for ( std::vector<Button *>::iterator iter = buttons.begin(); iter != buttons.end(); ++iter, ++iterableID ) {
        if ( iterableID == activeID ) {
            active = activeID;
            ( *iter )->PressDraw();
        }
        else {
            ( *iter )->ReleaseDraw();
        }
    }
}

uint RadioButtonsGroup::GetSelected( void )
{
    return active;
}
