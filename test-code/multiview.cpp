/**************************************************************************\
 *
 *  This file is part of the Coin family of 3D visualization libraries.
 *  Copyright (C) 1998-2004 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and / or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.  See the
 *  file LICENSE.GPL at the root directory of this source distribution
 *  for more details.
 *
 *  If you desire to use this library in software that is incompatible
 *  with the GNU GPL, and / or you would like to take advantage of the
 *  additional benefits with regard to our support services, please
 *  contact Systems in Motion about acquiring a Coin Professional
 *  Edition License.  See <URL:http://www.coin3d.org> for more
 *  information.
 *
 *  Systems in Motion, Abels gate 5, Teknobyen, 7030 Trondheim, NORWAY
 *  <URL:http://www.sim.no>, <mailto:support@sim.no>
 *
\**************************************************************************/

#include <Inventor/Win/viewers/SoWinExaminerViewer.h>
#include <Inventor/Win/viewers/SoWinPlaneViewer.h>
#include <Inventor/Win/SoWin.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/SoInput.h>

void
setUserData(
  HWND window,
  LONG data)
{
 SetWindowLong(window, GWL_USERDATA, data);
}

void
sizeWindow(
  HWND window,
  int width,
  int height)
{
 UINT flags = SWP_NOMOVE | SWP_NOZORDER;
 SetWindowPos(window, NULL, 0, 0, width, height, flags);
}

LRESULT CALLBACK
mainWindowProc(
  HWND window,
  UINT message,
  WPARAM wparam,
  LPARAM lparam)
{
 
 if ( message == WM_DESTROY ) {
  PostQuitMessage( 0 );
  return 0;
 }
  
  // Remove this if U want free floating viewers.
 if ( message == WM_SIZE ) {
    HWND * win = (HWND *)GetWindowLong(window, GWL_USERDATA);
  if (win) {
   MoveWindow(
        win[1],
        0,
        0,
        LOWORD(lparam) / 2,
        HIWORD(lparam) / 2,
        TRUE);
   MoveWindow(
        win[2],
        LOWORD(lparam) / 2,
        0,
        LOWORD(lparam) / 2,
        HIWORD(lparam) / 2,
        TRUE);
   MoveWindow(
        win[3],
        0,
        HIWORD(lparam) / 2,
        LOWORD(lparam) / 2,
        HIWORD(lparam) / 2,
        TRUE);
   MoveWindow(
        win[4],
        LOWORD(lparam) / 2,
        HIWORD(lparam) / 2,
        LOWORD(lparam) / 2,
        HIWORD(lparam) / 2,
        TRUE);
  }
 }
 return DefWindowProc(window, message, wparam, lparam);
}

LRESULT CALLBACK
viewerWindowProc(
  HWND window,
  UINT message,
  WPARAM wparam,
  LPARAM lparam)
{
  SoWinFullViewer * v =
      (SoWinFullViewer *)GetWindowLong(window, GWL_USERDATA);

  if (message == WM_SIZE) {
  if (v) v->setSize(SbVec2s(LOWORD(lparam), HIWORD(lparam)));
  }

  if (message == WM_DESTROY) {
    if (v) {
      SetWindowLong(window, GWL_USERDATA, NULL);
      if (v->getTypeId() == SoWinPlaneViewer::getClassTypeId())
        delete (SoWinPlaneViewer *)v;
      else
        delete (SoWinExaminerViewer *)v;
    }
  }
  
 return DefWindowProc(window, message, wparam, lparam);
}

HWND
createWindow(
 HINSTANCE instance,
 HWND parent,
 LPSTR wndclassname,
 UINT style,
 SbVec2s pos,
 SbVec2s size,
 WNDPROC proc
 )
{
  WNDCLASS windowclass;

  LPCTSTR icon = MAKEINTRESOURCE(IDI_APPLICATION);
  LPCTSTR cursor = MAKEINTRESOURCE(IDC_ARROW);
  HMENU menu = NULL;
  HBRUSH brush = (HBRUSH) GetSysColorBrush(COLOR_BTNFACE);

  windowclass.lpszClassName = wndclassname;
  windowclass.hInstance = instance;
  windowclass.lpfnWndProc = proc;
  windowclass.style = CS_OWNDC;
  windowclass.lpszMenuName = NULL;
  windowclass.hIcon = LoadIcon(NULL, icon);
  windowclass.hCursor = LoadCursor(instance, cursor);
  windowclass.hbrBackground = brush;
  windowclass.cbClsExtra = 0;
  windowclass.cbWndExtra = 4;

  RegisterClass(&windowclass);

  HWND window = CreateWindow(wndclassname,
                             wndclassname,
                             style,
                             pos[0],
                             pos[1],
                             size[0],
                             size[1],
                             parent,
                             NULL,
                             instance,
                             NULL);

  
  return window;
}

int WINAPI
WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nShowCmd)
{
  HWND win[5];

  // Uncomment the aditional styles if U want free floating viewers

  win[0] = createWindow(hInstance,
      NULL,
      "MainWindow",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      SbVec2s(CW_USEDEFAULT,CW_USEDEFAULT),
      SbVec2s(600,600),
      mainWindowProc);

  win[1] = createWindow(hInstance,
      win[0],
      "PlaneWindowA",
      WS_CHILD|WS_VISIBLE,
            // |WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,
      SbVec2s(0,0),
      SbVec2s(300,300),
      viewerWindowProc);

  win[2] = createWindow(hInstance,
      win[0],
      "ExaminerWindow",
      WS_CHILD|WS_VISIBLE,
            // |WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,
      SbVec2s(300,0),
      SbVec2s(300,300),
      viewerWindowProc);

  win[3] = createWindow(hInstance,
      win[0],
      "PlaneWindowB",
      WS_CHILD|WS_VISIBLE,
            // |WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,
      SbVec2s(0,300),
      SbVec2s(300,300),
      viewerWindowProc);

  win[4] = createWindow(hInstance,
      win[0],
      "PlaneWindowC",
      WS_CHILD|WS_VISIBLE,
            // |WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,
      SbVec2s(300,300),
      SbVec2s(300,300),
      viewerWindowProc);

  setUserData(win[0], (LONG)win);

  SoWin::init(win[0]);

  SoSeparator * s = new SoSeparator; // Scene root
  s->addChild(new SoCone);

  SoWinPlaneViewer * pviewer_a = new SoWinPlaneViewer(win[1]);
  setUserData(win[1], (LONG)pviewer_a);
  pviewer_a->setSceneGraph(s);
  pviewer_a->show();

  SoWinExaminerViewer * eviewer = new SoWinExaminerViewer(win[2]);
  setUserData(win[2], (LONG)eviewer);
  eviewer->setSceneGraph(s);
  eviewer->show();

  SoWinPlaneViewer * pviewer_b = new SoWinPlaneViewer(win[3]);
  setUserData(win[3], (LONG)pviewer_b);
  pviewer_b->setSceneGraph(s);
  pviewer_b->show();

  SoWinPlaneViewer * pviewer_c = new SoWinPlaneViewer(win[4]);
  setUserData(win[4], (LONG)pviewer_c);
  pviewer_c->setSceneGraph(s);
  pviewer_c->show();

  sizeWindow(win[0], 600, 600); // The viewers size to default size
                                // when created. We size them back.
  SoWin::mainLoop();

  return 0;
}
