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

#include <Inventor/Win/widgets/SoWinThumbWheel.h>
#include <Inventor/Win/widgets/SoAnyThumbWheel.h>
#include <Inventor/Win/SoWin.h>
#include <sowindefs.h>
#include <Inventor/Win/Win32API.h>

#include <math.h>
#include <assert.h>
#include <stdio.h>

// *************************************************************************

ATOM SoWinThumbWheel::wheelWndClassAtom = NULL;
int SoWinThumbWheel::wheelWidgetCounter = 0;

SoWinThumbWheel::SoWinThumbWheel(HWND parent,
                                  long id,
                                  int x,
                                  int y,
                                  char * name)
{
  this->constructor(SoWinThumbWheel::Vertical);
  RECT rect = { x, y, x + this->sizeHint().cx, y + this->sizeHint().cy };
  this->buildWidget(parent, rect, name);
  this->setId(id);
} // SoWinThumbWheel()

SoWinThumbWheel::SoWinThumbWheel(Orientation orientation,
                                  HWND parent,
                                  long id,
                                  int x,
                                  int y,
                                  char * name)
{
  this->constructor(orientation);
  RECT rect = { x, y, sizeHint().cx, sizeHint().cy };
  this->buildWidget(parent, rect, name);
  this->setId(id);
} // SoWinThumbWheel()


SoWinThumbWheel::~SoWinThumbWheel(void)
{
  delete this->wheel;
  if (this->pixmaps) {
    for (int i = 0; i < this->numPixmaps; i++)
      Win32::DeleteObject(this->pixmaps[i]);
    delete [] this->pixmaps;
  }
  if (IsWindow(this->wheelWindow))
    Win32::DestroyWindow(this->wheelWindow);
  if (IsWindow(this->labelWindow))
    Win32::DestroyWindow(this->labelWindow);
  if (SoWinThumbWheel::wheelWidgetCounter <= 0)
    Win32::UnregisterClass("ThumbWheel Widget", SoWin::getInstance());
} // ~SoWinThumbWheel()

SIZE
SoWinThumbWheel::sizeHint(void) const
{
  const int length = 118;
  int thick = 14;
  SIZE size;

  if (this->orient == SoWinThumbWheel::Horizontal) {
    size.cx = length;
    size.cy = thick;
    return size;
  }
  else {
    size.cx = thick;
    size.cy = length;
    return size;
  }
} // sizeHint()

HWND
SoWinThumbWheel::getWidget(void)
{
  return this->wheelWindow;
}

void
SoWinThumbWheel::setId(long id)
{
  (void)Win32::SetWindowLong(this->wheelWindow, GWL_ID, id);
}

long
SoWinThumbWheel::id(void) const
{
  return Win32::GetWindowLong(this->wheelWindow, GWL_ID);
}

void
SoWinThumbWheel::setOrientation(Orientation orientation)
{
  this->orient = orientation;
} // setOrientation()

SoWinThumbWheel::Orientation
SoWinThumbWheel::orientation(void) const
{
  return this->orient;
}

LRESULT CALLBACK
SoWinThumbWheel::onCreate(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  return 0;
}

LRESULT CALLBACK
SoWinThumbWheel::onSize(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  return 0;
}

LRESULT CALLBACK
SoWinThumbWheel::onPaint(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  PAINTSTRUCT ps;
  HDC hdc = Win32::BeginPaint(window, & ps);

  int w, d;
  if (this->orient == SoWinThumbWheel::Vertical) {
    w = this->width() - 2;
    d = this->height() - 2;
  } else {
    w = this->height() - 2;
    d = this->width() - 2;
  }

  // Handle resizing to too small dimensions gracefully.
  if ((d <= 0) || (w <= 0)) return 0;

  this->initWheel(d, w);

  int pixmap = this->wheel->getBitmapForValue(this->tempWheelValue,
                                               (this->state == SoWinThumbWheel::Disabled) ?
                                               SoAnyThumbWheel::DISABLED : SoAnyThumbWheel::ENABLED);
 
  this->blitBitmap(this->pixmaps[pixmap], hdc, 0, 0, this->width() - 2, this->height() - 2);
 
  this->currentPixmap = pixmap;

  Win32::EndPaint(window, & ps);
  return 0;
}

LRESULT CALLBACK
SoWinThumbWheel::onLButtonDown(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  if (this->state != SoWinThumbWheel::Idle)
    return 0;

 short x =  LOWORD(lparam);
 short y =  HIWORD(lparam);
 
 SetCapture(window);

  this->state = SoWinThumbWheel::Dragging;
  if (this->orient == SoWinThumbWheel::Vertical)
    this->mouseDownPos = y;
  else
    this->mouseDownPos = x;

  this->mouseLastPos = this->mouseDownPos;
 
  if ((this->viewerCB != NULL) && (this->viewer != NULL))
    this->viewerCB(this->viewer, NULL); // let CB know we want whateverWheelStart()

  return 0;
}

LRESULT CALLBACK
SoWinThumbWheel::onMouseMove(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  if (this->state != SoWinThumbWheel::Dragging)
    return 0;

  short x =  LOWORD(lparam);
  short y =  HIWORD(lparam);
 
  if (this->orient == SoWinThumbWheel::Vertical)
    this->mouseLastPos = y;
  else
    this->mouseLastPos = x;
 
  this->tempWheelValue =
    this->wheel->calculateValue(this->wheelValue,
                                 this->mouseDownPos,
                                 this->mouseLastPos - this->mouseDownPos);

  Win32::InvalidateRect(this->wheelWindow, NULL, FALSE);
 
  float * value = & this->tempWheelValue;
  if ((this->viewerCB != NULL) && (this->viewer != NULL)) {
    this->viewerCB(this->viewer, (void **) & value);
  }
  else {
    WPARAM wparam = Win32::GetWindowLong(window, GWL_ID);
    LPARAM lparam = (LPARAM) value;
    (void)SendMessage(GetParent(window), WM_THUMBWHEEL, wparam, lparam);
  }

 return 0;
}

LRESULT CALLBACK
SoWinThumbWheel::onLButtonUp(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  ReleaseCapture();
  if (this->state != SoWinThumbWheel::Dragging)
    return 0;

  this->wheelValue = this->tempWheelValue;
  this->mouseLastPos = this->mouseDownPos;
  this->state = SoWinThumbWheel::Idle;

 if ((this->viewerCB != NULL) && (this->viewer != NULL))  
    this->viewerCB(this->viewer, (void **) -1); // let CB know we want whateverWheelFinish()

  return 0;
}
   
LRESULT CALLBACK
SoWinThumbWheel::onDestroy(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  return 0;
}

LRESULT CALLBACK
SoWinThumbWheel::windowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  if (message == WM_CREATE) {
    CREATESTRUCT * createstruct;
    createstruct = (CREATESTRUCT *) lparam;

    (void)Win32::SetWindowLong(window, 0, (LONG) (createstruct->lpCreateParams));

    SoWinThumbWheel * object = (SoWinThumbWheel *)(createstruct->lpCreateParams);
    return object->onCreate(window, message, wparam, lparam);
  }

  SoWinThumbWheel * object = (SoWinThumbWheel *) Win32::GetWindowLong(window, 0);

  if (object && object->getWidget()) {

    switch (message)
      {
      case WM_SIZE:
        return object->onSize(window, message, wparam, lparam);

      case WM_PAINT:
        return object->onPaint(window, message, wparam, lparam);

      case WM_LBUTTONDOWN:
        return object->onLButtonDown(window, message, wparam, lparam);

      case WM_LBUTTONUP:
        return object->onLButtonUp(window, message, wparam, lparam);

      case WM_MOUSEMOVE:
        return object->onMouseMove(window, message, wparam, lparam);

      case WM_DESTROY:
        return object->onDestroy(window, message, wparam, lparam);
      }
  }
  return DefWindowProc(window, message, wparam, lparam);
}

int
SoWinThumbWheel::width(void)
{
 RECT rect;
 Win32::GetWindowRect(this->wheelWindow, & rect);
 return (rect.right - rect.left);
 
  //return this->sizeHint().cx;
}

int
SoWinThumbWheel::height(void)
{
  RECT rect;
  Win32::GetWindowRect(this->wheelWindow, & rect);
  return (rect.bottom - rect.top);
 
  //return this->sizeHint().cy;
}


void
SoWinThumbWheel::move(int x, int y, int width, int height)
{
  this->size(width, height);
  this->move(x, y);
}
  
void
SoWinThumbWheel::move(int x, int y)
{
  UINT flags = SWP_NOSIZE | SWP_NOZORDER;

  Win32::SetWindowPos(this->wheelWindow, NULL, x, y, 0, 0, flags);

  if (IsWindow(this->labelWindow)) {

    RECT rect;
    Win32::GetClientRect(this->labelWindow, & rect);
    
    if (this->orient == SoWinThumbWheel::Vertical) {
      Win32::SetWindowPos(this->labelWindow, NULL,
                           x + this->labelOffset.x,
                           y + this->labelOffset.y + this->height(),
                           0, 0, flags);
    }
    else {
      Win32::SetWindowPos(this->labelWindow, NULL,
                           x + this->labelOffset.x - rect.right,
                           y + this->labelOffset.y,
                           0, 0, flags);
    }
  }
}

void
SoWinThumbWheel::size(int width, int height)
{
  UINT flags = SWP_NOMOVE | SWP_NOZORDER;
  Win32::SetWindowPos(this->wheelWindow, NULL, 0, 0, width, height, flags);
  Win32::InvalidateRect(this->wheelWindow, NULL, FALSE);
  if (IsWindow(this->labelWindow))
    Win32::InvalidateRect(this->labelWindow, NULL, FALSE);
}

void
SoWinThumbWheel::show(void)
{
  (void)ShowWindow(this->wheelWindow, SW_SHOW);
  (void)ShowWindow(this->labelWindow, SW_SHOW);
}

void
SoWinThumbWheel::hide(void)
{
  (void)ShowWindow(this->wheelWindow, SW_HIDE);
  (void)ShowWindow(this->labelWindow, SW_HIDE);
}

void
SoWinThumbWheel::registerCallback(thumbWheelCB * func)
{
  this->viewerCB = func;
}

void
SoWinThumbWheel::registerViewer(SoWinFullViewer * viewer)
{
 this->viewer = viewer;
}

void
SoWinThumbWheel::constructor(Orientation orientation)
{
  this->orient = orientation;
  this->state = SoWinThumbWheel::Idle;
  this->wheelValue = this->tempWheelValue = 0.0f;
  this->wheel = new SoAnyThumbWheel;
  this->wheel->setMovement(SoAnyThumbWheel::UNIFORM);
  this->wheel->setGraphicsByteOrder(SoAnyThumbWheel::ARGB);
  this->wheel->setBoundaryHandling(SoAnyThumbWheel::MODULATE);
  this->pixmaps = NULL;
  this->numPixmaps = 0;
  this->currentPixmap = -1;
  this->viewer = NULL;
  this->viewerCB = NULL;
  this->labelWindow = NULL;
} // constructor()

HWND
SoWinThumbWheel::buildWidget(HWND parent, RECT rect, char * name)
{

  HMENU menu = NULL;
  LPSTR wndclassname = "ThumbWheel Widget";

  if (! SoWinThumbWheel::wheelWndClassAtom) {

    WNDCLASS windowclass;

    windowclass.lpszClassName = wndclassname;
    windowclass.hInstance = SoWin::getInstance();
    windowclass.lpfnWndProc = SoWinThumbWheel::windowProc;
    windowclass.style = CS_HREDRAW | CS_VREDRAW;
    windowclass.lpszMenuName = NULL;
    windowclass.hIcon = NULL;
    windowclass.hCursor = NULL;
    windowclass.hbrBackground = NULL;
    windowclass.cbClsExtra = 0;
    windowclass.cbWndExtra = 4;

    SoWinThumbWheel::wheelWndClassAtom = Win32::RegisterClass(& windowclass);
    
  }

  SoWinThumbWheel::wheelWidgetCounter++;

  this->wheelWindow = CreateWindow(wndclassname,
                                    wndclassname,
                                    WS_VISIBLE |
                                    WS_CLIPCHILDREN |
                                    WS_CLIPSIBLINGS |
                                    WS_CHILD |
                                    WS_BORDER,
                                    rect.left,
                                    rect.top,
                                    rect.right,
                                    rect.bottom,
                                    parent,
                                    menu,
                                    SoWin::getInstance(),
                                    this);

  assert(IsWindow(this->wheelWindow));

  if (name)
    this->labelWindow = createLabel(parent, rect.right, rect.bottom, name);
  this->setLabelOffset(0, 0);
 
  return this->wheelWindow;
}

void
SoWinThumbWheel::initWheel(int diameter, int width)
{
  int d, w;
  this->wheel->getSize(d, w);
  if (d == diameter && w == width) return;

  this->wheel->setSize(diameter, width);

  int pwidth = width;
  int pheight = diameter;
  if (this->orient == Horizontal) {
    pwidth = diameter;
    pheight = width;
  }

  if (this->pixmaps != NULL) {
    for (int i = 0; i < this->numPixmaps; i++) {
      Win32::DeleteObject(this->pixmaps[i]);
      this->pixmaps[i] = NULL;
    }
    delete [] this->pixmaps;
  }

  this->numPixmaps = this->wheel->getNumBitmaps();
  void * bits = NULL;

  this->pixmaps = new HBITMAP[numPixmaps];

  for (int i = 0; i < this->numPixmaps; i++) {
    this->pixmaps[i] = this->createDIB(pwidth, pheight, 32, &bits);
    this->wheel->drawBitmap(i, bits, (this->orient == Vertical) ?
                             SoAnyThumbWheel::VERTICAL : SoAnyThumbWheel::HORIZONTAL);
  }
} // initWheel()

// *************************************************************************

void
SoWinThumbWheel::setEnabled(bool enable)
{  
  if (enable)
    this->state = SoWinThumbWheel::Idle;
  else
    this->state = SoWinThumbWheel::Disabled;
  Win32::InvalidateRect(this->wheelWindow, NULL, FALSE);
  if (IsWindow(this->labelWindow))
    Win32::EnableWindow(this->labelWindow, enable);
} // setEnabled()

bool
SoWinThumbWheel::isEnabled(void) const
{
  return (this->state != SoWinThumbWheel::Disabled);
} // isEnabled()

void
SoWinThumbWheel::setValue(float value)
{
  this->wheelValue = this->tempWheelValue = value;
  this->mouseDownPos = this->mouseLastPos;
  Win32::InvalidateRect(this->wheelWindow, NULL, FALSE);
} // setValue()

float
SoWinThumbWheel::value(void) const
{
  return this->wheelValue;
}

float
SoWinThumbWheel::tmpValue(void) const
{
  return this->tempWheelValue;
}

void
SoWinThumbWheel::setLabelText(char * text)
{
  assert(IsWindow(this->wheelWindow));
  
  if (IsWindow(this->labelWindow)) {
    Win32::SetWindowText(this->labelWindow, text);
  }
  else {
    RECT rect;
    HWND parent = GetParent(this->wheelWindow);
    Win32::GetWindowRect(this->wheelWindow, & rect);
    this->labelWindow = createLabel(parent, rect.right + this->labelOffset.x,
                                     rect.bottom + labelOffset.y, text);
  }
      
  int len = strlen(text);
  HDC hdc = Win32::GetDC(this->labelWindow);
  SIZE textSize;
  Win32::GetTextExtentPoint(hdc, text, len, & textSize);
  
  UINT flags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW;
  Win32::SetWindowPos(this->labelWindow, NULL, 0, 0,
                       textSize.cx + 2, textSize.cy, flags);
}

void
SoWinThumbWheel::setLabelOffset(int x, int y)
{
  this->labelOffset.x = x;
  this->labelOffset.y = y;
}

SIZE
SoWinThumbWheel::getLabelSize(void)
{
  RECT rect;
  Win32::GetWindowRect(this->labelWindow, & rect);
  SIZE size = { rect.right - rect.left, rect.bottom - rect.top };
  return (size);
}

// *************************************************************************

void
SoWinThumbWheel::setRangeBoundaryHandling(boundaryHandling handling)
{
  switch (handling) {
  case CLAMP:
    this->wheel->setBoundaryHandling(SoAnyThumbWheel::CLAMP);
    break;
  case MODULATE:
    this->wheel->setBoundaryHandling(SoAnyThumbWheel::MODULATE);
    break;
  case ACCUMULATE:
    this->wheel->setBoundaryHandling(SoAnyThumbWheel::ACCUMULATE);
    break;
  default:
    assert(0 && "impossible");
  }
} // setRangeBoundaryHandling()

// *************************************************************************

SoWinThumbWheel::boundaryHandling
SoWinThumbWheel::getRangeBoundaryHandling(void) const
{
  switch (this->wheel->getBoundaryHandling()) {
  case SoAnyThumbWheel::CLAMP:
    return CLAMP;
  case SoAnyThumbWheel::MODULATE:
    return MODULATE;
  case SoAnyThumbWheel::ACCUMULATE:
    return ACCUMULATE;
  default:
    assert(0 && "impossible");
  }
  return CLAMP; // never reached
} // getRangeBoundaryHandling()

HWND
SoWinThumbWheel::createLabel(HWND parent, int x, int y, char * text)
{
  assert(IsWindow(parent));
  SIZE textSize = this->getTextSize(parent, text); // FIXME: assumes the same font as parent
  HWND hwnd = CreateWindow("STATIC",
                            (text ? text : " "),
                            WS_VISIBLE | WS_CHILD | SS_CENTER,
                            x, y,
                            textSize.cx + 2, textSize.cy, // SIZE
                            parent,
                            NULL,
                            SoWin::getInstance(),
                            NULL);
 assert(IsWindow(hwnd));
 return hwnd;
}

HBITMAP
SoWinThumbWheel::createDIB(int width, int height, int bpp, void ** bits) // 16||24||32 bpp
{
  assert(bpp > 8);

  HBITMAP bitmap = NULL;
  HDC hdc = CreateCompatibleDC(NULL);
  assert(hdc!=NULL && "CreateCompatibleDC() failed -- investigate");
  int heapspace = sizeof(BITMAPINFOHEADER);

  HANDLE heap = GetProcessHeap();
  BITMAPINFO * format = (BITMAPINFO *) HeapAlloc(heap, 0, heapspace);
 
  BITMAPINFOHEADER * header = (BITMAPINFOHEADER *) format;
  header->biSize = sizeof(BITMAPINFOHEADER);
  header->biWidth = width;
  header->biHeight = -height;
  header->biPlanes = 1;
  header->biBitCount = bpp;
  header->biCompression = BI_RGB;
  header->biSizeImage = 0;
  header->biXPelsPerMeter = 0;
  header->biYPelsPerMeter = 0;
  header->biClrUsed = 0;
  header->biClrImportant = 0;

  UINT flag = DIB_RGB_COLORS;
  bitmap = CreateDIBSection(hdc, format, flag, (void **) bits, NULL, 0);
  assert(* bits);

  HeapFree(heap, 0, format);
  DeleteDC(hdc);

  return bitmap;
}

void
SoWinThumbWheel::blitBitmap(HBITMAP bitmap, HDC dc, int x,int y, int width, int height) const
{
  HDC memorydc = CreateCompatibleDC(dc);
  assert(memorydc!=NULL && "CreateCompatibleDC() failed -- investigate");
  HBITMAP oldBitmap = (HBITMAP) SelectObject(memorydc, bitmap);
  Win32::BitBlt(dc, x, y, width, height, memorydc, 0, 0, SRCCOPY);
  Win32::SelectObject(memorydc, oldBitmap);
  Win32::DeleteDC(memorydc);
}

SIZE
SoWinThumbWheel::getTextSize(HWND window, char * text)
{
  assert(IsWindow(window));
  
  int len = strlen(text);
  HDC hdc = Win32::GetDC(window);

  SIZE size;
  Win32::GetTextExtentPoint(hdc, text, len, & size);
  return size;
}
