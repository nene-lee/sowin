/**************************************************************************
 *
 *  This file is part of the Coin SoWin GUI binding library.
 *  Copyright (C) 2000 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  version 2.1 as published by the Free Software Foundation.  See the
 *  file LICENSE.LGPL at the root directory of the distribution for
 *  more details.
 *
 *  If you want to use Coin SoWin for applications not compatible with the
 *  LGPL, please contact SIM to aquire a Professional Edition License.
 *
 *  Systems in Motion, Prof Brochs gate 6, N-7030 Trondheim, NORWAY
 *  http://www.sim.no/ support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
 **************************************************************************/

static const char rcsid[] =
  "$Id$";

#include <windows.h>

#include <Inventor/SoLists.h>
#include <Inventor/errors/SoDebugError.h>

#include <sowindefs.h>
#include <Inventor/Win/widgets/SoWinPopupMenu.h>
#include <Inventor/Win/Win32API.h>

/*!
  \class SoWinPopupMenu Inventor/Win/widgets/SoWinPopupMenu.h
  \brief The SoWinPopupMenu class implements a common interface for popup
  menu management for all the Coin GUI toolkit libraries.
*/

// *************************************************************************

struct MenuRecord {
  int menuid;
  char * name;
  char * title;
  HMENU menu;
  HMENU parent;
}; // struct MenuRecord

struct ItemRecord {
  int itemid;
  int flags;
  char * name;
  char * title;
  HMENU parent;
}; // struct ItemRecord

#define ITEM_TOGGLE       0x0001
#define ITEM_MARKED       0x0002
#define ITEM_SEPARATOR    0x0004
#define ITEM_ENABLED      0x0008

// *************************************************************************

SoWinPopupMenu::SoWinPopupMenu( void )
{
  this->menus = new SbPList;
  this->items = new SbPList;

  this->notify = FALSE;
  this->selectedItem = -1;
} // SoWinPopupMenu()

SoWinPopupMenu::~SoWinPopupMenu( void )
{
  const int numMenus = this->menus->getLength();
  HMENU popup = NULL;
  int i;
  for ( i = 0; i < numMenus; i++ ) {
    MenuRecord * rec = ( MenuRecord * ) ( * this->menus )[i];
    if ( rec->menuid == 0 ) popup = rec->menu;
    delete [] rec->name;
    delete [] rec->title;
    if ( rec->parent == NULL ) delete rec->menu; // menu not attached
    delete rec;
  }

  const int numItems = this->items->getLength( );
  for ( i = 0; i < numItems; i++ ) {
    ItemRecord * rec = ( ItemRecord * ) ( * this->items )[i];
    delete [] rec->name;
    delete [] rec->title;
    delete rec;
  }

  // delete root popup menu
  delete popup;
} // ~SoWinPopupMenu()

// *************************************************************************

int
SoWinPopupMenu::newMenu( const char * name, int menuid )
{
  int id = menuid;
  if ( id == -1 ) {
    id = 1;
    while ( this->getMenuRecord( id ) != NULL ) id++;
  } else {
    assert( this->getMenuRecord( id ) == NULL &&
            "requested menuid already taken" );
  }
  // id contains ok ID
  MenuRecord * rec = createMenuRecord( name );
  rec->menuid = id;
  this->menus->append( ( void * ) rec );
  return id;
} // newMenu()

int
SoWinPopupMenu::getMenu( const char * name )
{
  const int numMenus = this->menus->getLength();
  int i;
  for ( i = 0; i < numMenus; i++ ) {
    if ( strcmp( ( ( MenuRecord * ) ( * this->menus )[i] )->name, name ) == 0 ) {
      return ( ( MenuRecord * ) ( * this->menus )[i] )->menuid;
    }
  }
  return -1;
} // getMenu()

void
SoWinPopupMenu::setMenuTitle( int menuid, const char * title )
{
  MenuRecord * rec = this->getMenuRecord( menuid );
  assert( rec != NULL && "no such menu" );
  delete [] rec->title;
  rec->title = strcpy( new char [strlen(title)+1], title );

  if ( rec->parent )
    Win32::ModifyMenu( rec->parent, rec->menuid, MF_BYPOSITION | MF_STRING, rec->menuid, rec->title );

} // setMenuTitle()

const char *
SoWinPopupMenu::getMenuTitle( int menuid )
{
  MenuRecord * rec = this->getMenuRecord( menuid );
  assert( rec != NULL && "no such menu" );
  return rec->title;
} // getMenuTitle()

// *************************************************************************

int
SoWinPopupMenu::newMenuItem( const char * name, int itemid )
{
  int id = itemid;
  if ( id == -1 ) {
    id = 1;
    while ( this->getItemRecord( itemid ) != NULL ) id++;
  } else {
    if ( this->getItemRecord( itemid ) != NULL ) {
#if SOWIN_DEBUG
      SoDebugError::postInfo( "SoWinPopupMenu::newMenuItem",
                              "requested itemid already taken" );
#endif // SOWIN_DEBUG
      return -1;
    }
  }
  ItemRecord * rec = createItemRecord( name );
  rec->itemid = id;
  this->items->append( rec );
  return id;
} // newMenuItem()

int
SoWinPopupMenu::getMenuItem( const char * name )
{
  const int numItems = this->items->getLength( );
  int i;
  for ( i = 0; i < numItems; i++ )
    if ( strcmp( ( ( ItemRecord * ) ( * this->items)[i] )->name, name ) == 0 )
      return ( ( ItemRecord * ) ( * this->items)[i] )->itemid;
  return -1;
} // getMenuItem()

void
SoWinPopupMenu::setMenuItemTitle( int itemid, const char * title )
{
  ItemRecord * rec = this->getItemRecord( itemid );
  assert( rec != NULL && "no such menu" );
  delete [] rec->title;
  rec->title = strcpy( new char [strlen(title)+1], title );

  if ( rec->parent )
    Win32::ModifyMenu( rec->parent, rec->itemid, MF_BYCOMMAND | MF_STRING, rec->itemid, rec->title );
} // setMenuItemTitle()

const char *
SoWinPopupMenu::getMenuItemTitle( int itemid )
{
  ItemRecord * rec = this->getItemRecord( itemid );
  assert( rec != NULL && "no such menu" );
  return rec->title;
} // getMenuItemTitle()

void
SoWinPopupMenu::setMenuItemEnabled( int itemid, SbBool enabled )
{
  ItemRecord * rec = this->getItemRecord( itemid );

  assert( rec && "could not find item record" );
  assert( rec->parent && "a menuitem must have a parent to be enabled/disabled" );
  assert( IsMenu( rec->parent ) );
  
#if 1 // old code
  if ( enabled )
    rec->flags |= ITEM_ENABLED;
  else
    rec->flags &= ~ITEM_ENABLED;

 Win32::EnableMenuItem( rec->parent, rec->itemid, MF_BYCOMMAND | ( enabled ? MF_ENABLED : MF_GRAYED ) );
#else // new code, but FIXME: it's crashing. 20010810 mortene.
  MENUITEMINFO info;
  
  info.cbSize = sizeof( MENUITEMINFO );
  info.fMask = MIIM_STATE;
  
  if ( enabled ) {
    rec->flags |= ITEM_ENABLED;
    info.fState = MFS_ENABLED | MFS_GRAYED;
  }
  else {
    rec->flags &= ~ITEM_ENABLED;    
    info.fState = MFS_DISABLED;
  }
  
  Win32::SetMenuItemInfo( rec->parent, rec->itemid, FALSE, & info );
#endif
} // setMenuItemEnabled()

SbBool
SoWinPopupMenu::getMenuItemEnabled( int itemid )
{
  ItemRecord * rec = this->getItemRecord( itemid );
  
  assert( rec && "could not find item record" );
  assert( IsMenu( rec->parent ) );

  //MENUITEMINFO  menuiteminfo;
  //memset( ( void * ) & menuiteminfo, 0, sizeof( menuiteminfo ) );
  //Win32::GetMenuItemInfo( rec->parent, rec->itemid, TRUE, & menuiteminfo );
  //return ( menuiteminfo.fState & MFS_ENABLED ) ? TRUE : FALSE;
 
  return ( rec->flags & ITEM_ENABLED ? TRUE : FALSE );
} // getMenuItemEnabled()

void
SoWinPopupMenu::_setMenuItemMarked( int itemid, SbBool marked )
{
  ItemRecord * rec = this->getItemRecord( itemid );

  assert( rec != NULL && "no such menu" );
  assert( IsMenu( rec->parent ) );

  rec->flags |= ITEM_TOGGLE;

  MENUITEMINFO info;
  
  info.cbSize = sizeof( MENUITEMINFO );
  info.fMask = MIIM_STATE;
  
  if ( marked ) {
    rec->flags |= ITEM_MARKED;
    info.fState = MFS_CHECKED;
  }
  else {
    rec->flags &= ~ITEM_MARKED;    
    info.fState = MFS_UNCHECKED;
  }
  
  Win32::SetMenuItemInfo( rec->parent, rec->itemid, FALSE, & info );

} // setMenuItemMarked()

SbBool
SoWinPopupMenu::getMenuItemMarked( int itemid )
{
  ItemRecord * rec = this->getItemRecord( itemid );
  assert( rec != NULL && "no such menu" );
  assert( rec->parent != NULL );
  
  MENUITEMINFO info;

  info.cbSize = sizeof( MENUITEMINFO );
  info.fMask = MIIM_STATE;
  
  Win32::GetMenuItemInfo( rec->parent, rec->itemid, FALSE, & info );

  return ( info.fState & MFS_CHECKED ? TRUE : FALSE );
} // getMenuItemMarked()

// *************************************************************************

void
SoWinPopupMenu::addMenu( int menuid, int submenuid, int pos )
{
  MenuRecord * super = this->getMenuRecord( menuid );
  MenuRecord * sub = this->getMenuRecord( submenuid );
  assert( super != NULL && sub != NULL && "no such menu" );

  MENUITEMINFO menuiteminfo;
  memset( ( void * ) & menuiteminfo, 0, sizeof( menuiteminfo ) );
  menuiteminfo.cbSize = sizeof( menuiteminfo );
  menuiteminfo.fMask = MIIM_SUBMENU | MIIM_TYPE;
  menuiteminfo.fType = MFT_STRING;
  menuiteminfo.hSubMenu = sub->menu;
  menuiteminfo.dwTypeData = sub->title;
  menuiteminfo.cch = strlen( sub->title );

  if ( pos == -1 )
    Win32::InsertMenuItem( super->menu, sub->menuid, FALSE, & menuiteminfo );
  else
    Win32::InsertMenuItem( super->menu, pos, TRUE, & menuiteminfo );
  sub->parent = super->menu;
} // addMenu()

void
SoWinPopupMenu::addMenuItem( int menuid, int itemid, int pos )
{
  MenuRecord * menu = this->getMenuRecord( menuid );
  ItemRecord * item = this->getItemRecord( itemid );
  assert( menu != NULL && item != NULL && "no such menu" );
  
  Win32::InsertMenu( menu->menu, pos, MF_BYPOSITION | MF_STRING, item->itemid, item->title );

  item->parent = menu->menu;
  if ( item->flags & ITEM_MARKED )
    Win32::CheckMenuItem( item->parent, item->itemid, MF_BYCOMMAND | MF_CHECKED );
} // addMenuItem()

void
SoWinPopupMenu::addSeparator( int menuid, int pos )
{
  MenuRecord * menu = this->getMenuRecord( menuid );
  assert( menu != NULL && "no such menu" );
  ItemRecord * rec = createItemRecord( "separator" );
 
  Win32::InsertMenu( menu->menu, pos, MF_BYPOSITION | MF_SEPARATOR, pos, NULL );
  rec->flags |= ITEM_SEPARATOR;
  this->items->append( rec );
} // addSeparator()

void
SoWinPopupMenu::removeMenu( int menuid )
{
  MenuRecord * rec = this->getMenuRecord( menuid );
  assert( rec != NULL && "no such menu" );

  // FIXME: just assumes root-menu has id==0. Bad. 20010810 mortene.
  if ( rec->menuid == 0 ) {
#if SOWIN_DEBUG
    SoDebugError::postInfo( "SoWinPopupMenu::removeMenu", "can't remove root" );
#endif // SOWIN_DEBUG
    return;
  }
  if ( rec->parent == NULL ) {
#if SOWIN_DEBUG
    SoDebugError::postInfo( "SoWinPopupMenu::removeMenu", "menu not attached" );
#endif // SOWIN_DEBUG
    return;
  }
  Win32::RemoveMenu( rec->menu, rec->menuid, MF_BYCOMMAND );
  rec->parent = NULL;
} // removeMenu()

void
SoWinPopupMenu::removeMenuItem( int itemid )
{
  ItemRecord * rec = this->getItemRecord( itemid );
  assert( rec != NULL && "no such menu" );
  if ( rec->parent == NULL ) {
#if SOWIN_DEBUG
    SoDebugError::postInfo( "SoWinPopupMenu::removeMenuItem", "item not attached" );
#endif // SOWIN_DEBUG
    return;
  }
  Win32::RemoveMenu( rec->parent, rec->itemid, MF_BYCOMMAND );
  rec->parent = NULL;
} // removeMenuItem()

// *************************************************************************

void
SoWinPopupMenu::popUp( HWND inside, int x, int y )
{

  MenuRecord * menurec = this->getMenuRecord( 0 );
  this->selectedItem = TrackPopupMenu( menurec->menu,
                                       TPM_LEFTALIGN |
                                       TPM_TOPALIGN |
                                       TPM_RIGHTBUTTON |
                                       TPM_RETURNCMD |
                                       ( this->notify ? 0 : TPM_NONOTIFY ),
                                       x,
                                       y,
                                       0,
                                       inside,
                                       NULL );
  
  if ( this->selectedItem == 0 )
    return;
  
  ItemRecord * itemrec = this->getItemRecord( this->selectedItem );
  assert( itemrec != NULL );
  
} // popUp()

int
SoWinPopupMenu::getSelectedItem( void )
{
 return ( this->selectedItem );
} // getSelectedItem()

void
SoWinPopupMenu::setNotify( SbBool enable )
{
 this->notify = enable;
} // setNotify()

// *************************************************************************

MenuRecord *
SoWinPopupMenu::getMenuRecord( int menuid )
{
  const int numMenus = this->menus->getLength( );
  int i;
  for ( i = 0; i < numMenus; i++ )
    if ( ( ( MenuRecord *) ( * this->menus)[i] )->menuid == menuid )
      return ( MenuRecord * ) ( * this->menus )[i];
  return ( MenuRecord * ) NULL;
} // getMenuRecord()

ItemRecord *
SoWinPopupMenu::getItemRecord( int itemid )
{
  const int numItems = this->items->getLength( );
  int i;
 
  for ( i = 0; i < numItems; i++ )
    if ( ( ( ItemRecord * ) ( * this->items)[i] )->itemid == itemid )
      return ( ItemRecord * ) ( * this->items)[i];

  return ( ItemRecord * ) NULL;
} // getItemRecord()

// *************************************************************************

MenuRecord *
SoWinPopupMenu::createMenuRecord( const char * name )
{
  MenuRecord * rec = new MenuRecord;
  rec->menuid = -1;
  rec->name = strcpy( new char [strlen( name ) + 1], name );
  rec->title = strcpy( new char [strlen( name ) + 1], name );
  rec->menu = CreatePopupMenu( );
  rec->parent = NULL;
  return rec;
} // create()

ItemRecord *
SoWinPopupMenu::createItemRecord( const char * name )
{
  ItemRecord * rec = new ItemRecord;
  rec->itemid = -1;
  rec->flags = 0;
  rec->name = strcpy( new char [strlen( name ) + 1], name );
  rec->title = strcpy( new char [strlen( name ) + 1], name );
  rec->parent = NULL;
  return rec;
} // create()

// *************************************************************************

void
SoWinPopupMenu::itemActivation( int itemid )
{
  inherited::invokeMenuSelection( itemid );
} // menuSelection()

// *************************************************************************
