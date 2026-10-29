// Copyright (C) 2016 Pavel Demin
// SPDX-License-Identifier: BSL-1.0

#ifndef Indicator_h
#define Indicator_h

#include <QtWidgets/QFrame>
#include <QtCore/QList>

class QHBoxLayout;
class CustomDigit;

class Indicator: public QFrame
{
  Q_OBJECT

public:
  Indicator(QWidget *parent = 0);
  virtual ~Indicator();

public slots:
  qint64 value() {return m_Value;};
  void setFont(QFont font);
  void setFontSize(int size);
  void setSize(int size);
  void setValue(qint64 value);
  void setValueMin(qint64 value);
  void setValueMax(qint64 value);
  void setDeltaMin(qint64 delta);
  void applyDelta(qint64 delta);

signals:
  void valueChanged(qint64 value);

private:
  QHBoxLayout *m_Layout;
  //the digit labels, most significant first (children of this frame)
  QList<CustomDigit *> m_Digits;
  QFont m_Font;
  int m_Size;
  qint64 m_Value;
  qint64 m_ValueMin;
  qint64 m_ValueMax;
};

#endif
