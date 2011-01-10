//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: canvas.h,v 1.3.2.8 2009/02/02 21:38:01 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "citem.h"
#include "view.h"
#include "tools.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

class QMenu;

//---------------------------------------------------------
//   Canvas
//---------------------------------------------------------

class Canvas : public View
{
    Q_OBJECT
    int _canvasTools;
    QTimer *_scrollTimer;

    bool _doScroll;
    int _scrollSpeed;

    QPoint _evPos;
    bool _canScrollLeft;
    bool _canScrollRight;
    bool _canScrollUp;
    bool _canScrollDown;
protected:

    enum DragMode
    {
        DRAG_OFF, DRAG_NEW,
        DRAG_MOVE_START, DRAG_MOVE,
        DRAG_COPY_START, DRAG_COPY,
        DRAG_CLONE_START, DRAG_CLONE,
        DRAGX_MOVE, DRAGY_MOVE,
        DRAGX_COPY, DRAGY_COPY,
        DRAGX_CLONE, DRAGY_CLONE,
        DRAG_DELETE,
        DRAG_RESIZE, DRAG_LASSO_START, DRAG_LASSO,
    };

    enum DragType
    {
        MOVE_MOVE, MOVE_COPY, MOVE_CLONE
    };

    enum HScrollDir
    {
        HSCROLL_NONE, HSCROLL_LEFT, HSCROLL_RIGHT
    };

    enum VScrollDir
    {
        VSCROLL_NONE, VSCROLL_UP, VSCROLL_DOWN
    };

    CItemList _items;
    CItemList _moving;
    CItem* _curItem;
    Part* _curPart;
    int _curPartId;

    DragMode _drag;
    QRect _lasso;
    QPoint _start;
    Tool _tool;
    unsigned _pos[3];

    HScrollDir _hscrollDir;
    VScrollDir _vscrollDir;
    int _button;
    Qt::KeyboardModifiers _keyState;
    QMenu* _itemPopupMenu;
    QMenu* _canvasPopupMenu;

    void setCursor();
    virtual void viewKeyPressEvent(QKeyEvent* event);
    virtual void viewMousePressEvent(QMouseEvent* event);
    virtual void viewMouseMoveEvent(QMouseEvent*);
    virtual void viewMouseReleaseEvent(QMouseEvent*);
    virtual void draw(QPainter&, const QRect&);
    virtual void wheelEvent(QWheelEvent* e);

    virtual void mousePress(QMouseEvent*)
    {
    }
    virtual void keyPress(QKeyEvent*);
    virtual void mouseMove(const QPoint&) = 0;

    virtual void mouseRelease(const QPoint&)
    {
    }
    virtual void drawCanvas(QPainter&, const QRect&) = 0;
    virtual void drawItem(QPainter&, const CItem*, const QRect&) = 0;
    virtual void drawMoving(QPainter&, const CItem*, const QRect&) = 0;
    virtual void updateSelection() = 0;
    virtual QPoint raster(const QPoint&) const = 0;
    virtual int y2pitch(int) const = 0; //CDW
    virtual int pitch2y(int) const = 0; //CDW

    virtual void moveCanvasItems(CItemList&, int, int, DragType, int*) = 0;
    // Changed by T356.
    //virtual bool moveItem(CItem*, const QPoint&, DragType, int*) = 0;
    virtual bool moveItem(CItem*, const QPoint&, DragType) = 0;
    virtual CItem* newItem(const QPoint&, int state) = 0;
    virtual void resizeItem(CItem*, bool noSnap = false) = 0;
    virtual void newItem(CItem*, bool noSnap = false) = 0;
    virtual bool deleteItem(CItem*) = 0;
    virtual void startUndo(DragType) = 0;

    virtual void endUndo(DragType, int flags) = 0;
    int getCurrentDrag();

    /*!
       \brief Virtual member

       Implementing class is responsible for creating a popup to be shown when the user rightclicks an item on the Canvas
       \param item The canvas item that is rightclicked
       \return A QPopupMenu*
     */
    virtual QMenu* genItemPopup(CItem* /*item*/)
    {
        return 0;
    }

    /*!
       \brief Pure virtual member

       Implementing class is responsible for creating a popup to be shown when the user rightclicks an empty region of the canvas
       \return A QPopupMenu*
     */
    QMenu* genCanvasPopup();

    /*!
       \brief Virtual member

       This is the function called when the user has selected an option in the popupmenu generated by genItemPopup()
       \param item the canvas item the whole thing is about
       \param n Command type
       \param pt I think this is the position of the pointer when right mouse button was pressed
     */
    virtual void itemPopup(CItem* /*item */, int /*n*/, const QPoint& /*pt*/)
    {
    }
    void canvasPopup(int);

    virtual void startDrag(CItem*, bool)
    {
    }

    // selection
    virtual void deselectAll();
    virtual void selectItem(CItem* e, bool);

    virtual void deleteItem(const QPoint&);

    // moving
    void startMoving(const QPoint&, DragType);

    void moveItems(const QPoint&, int dir, bool rasterize = true);
    void endMoveItems(const QPoint&, DragType, int dir);

    virtual void selectLasso(bool toggle);

    virtual void itemPressed(const CItem*)
    {
    }

    virtual void itemReleased(const CItem*, const QPoint&)
    {
    }

    virtual void itemMoved(const CItem*, const QPoint&)
    {
    }

    virtual void curPartChanged()
    {
    }

    CItemList getItemlistForCurrentPart();


public slots:
    void setTool(int t);
    void setPos(int, unsigned, bool adjustScrollbar);
    void scrollTimerDone(void);
    void redirectedWheelEvent(QWheelEvent*);

signals:
    void followEvent(int);
    void toolChanged(int);
    void verticalScroll(unsigned);
    void horizontalScroll(unsigned);
    void horizontalScrollNoLimit(unsigned);
public:
    Canvas(QWidget* parent, int sx, int sy, const char* name = 0);
    bool isSingleSelection();
    int selectionSize();

    Tool tool() const
    {
        return _tool;
    }

    Part* part() const
    {
        return _curPart;
    }
    void setCurrentPart(Part*);

    void setCanvasTools(int n)
    {
        _canvasTools = n;
    }
};
#endif

