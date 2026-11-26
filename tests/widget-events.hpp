#pragma once

// These tests need ordinary Qt event delivery, not QtTest (which OBS's
// pinned native dependency bundles do not ship).
#include <QApplication>
#include <QEventLoop>
#include <QMouseEvent>
#include <QTimer>
#include <QWidget>

namespace TestEvents {
inline void wait(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, Qt::PreciseTimer, &loop, &QEventLoop::quit);
    loop.exec();
}

inline void mouse(QWidget *widget, QEvent::Type type, Qt::MouseButton button)
{
    const QPoint local = widget->rect().center();
    const QPoint global = widget->mapToGlobal(local);
    const QPoint window = widget->mapTo(widget->window(), local);
    const Qt::MouseButtons buttons = type == QEvent::MouseButtonPress
        ? Qt::MouseButtons(button) : Qt::NoButton;
    QMouseEvent event(type, QPointF(local), QPointF(window), QPointF(global),
                      button, buttons, Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
}

inline void mousePress(QWidget *widget, Qt::MouseButton button)
{
    mouse(widget, QEvent::MouseButtonPress, button);
}

inline void mouseRelease(QWidget *widget, Qt::MouseButton button)
{
    mouse(widget, QEvent::MouseButtonRelease, button);
}

inline void mouseClick(QWidget *widget, Qt::MouseButton button)
{
    mousePress(widget, button);
    mouseRelease(widget, button);
}
}
