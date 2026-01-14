#include "XGraphicsScene.h"

#include <qpainter.h>
#include <qdebug.h>

#include <QGraphicsSceneMouseEvent>

XGraphicsScene::XGraphicsScene(QObject* parent)
	: QGraphicsScene(parent), m_isDrawing(false)
{}

XGraphicsScene::~XGraphicsScene()
{}

void XGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    //if (event->button() == Qt::LeftButton) {
    //    qDebug() << "鼠标左键按下";
    //    // 记录起始点（转换为场景坐标）
    //    m_startPoint = event->scenePos();
    //    m_endPoint = m_startPoint;
    //    m_isDrawing = true;
    //    m_currentRect = QRectF(m_startPoint, m_endPoint);
    //    update(); // 请求重绘，触发drawForeground
    //}
    //// 重要：调用基类事件，确保场景原有的交互（如项目选择）仍能正常工作
    //QGraphicsScene::mousePressEvent(event);
}

void XGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    //if (m_isDrawing) {
    //    // 更新当前终点并计算矩形
    //    m_endPoint = event->scenePos();
    //    // 确保矩形为正（即起始点和终点顺序无关）
    //    m_currentRect = QRectF(m_startPoint, m_endPoint).normalized();
    //    update(); // 请求重绘，更新临时矩形显示
    //}
    //QGraphicsScene::mouseMoveEvent(event);
}

void XGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    //if (m_isDrawing && event->button() == Qt::LeftButton) {
    //    qDebug() << "鼠标左键松开";
    //    m_isDrawing = false;
    //    m_endPoint = event->scenePos();
    //    m_currentRect = QRectF(m_startPoint, m_endPoint).normalized();

    //    // 绘制完成，发出信号（例如，用于后续ROI处理）
    //    if (!m_currentRect.isEmpty() && m_currentRect.isValid()) {
    //        qDebug() << "选中的ROI区域为：" << m_currentRect;
    //        emit roiSelected(m_currentRect);
    //    }
    //    // 清除临时矩形
    //    //m_currentRect = QRectF();
    //    //update();
    //}
    //QGraphicsScene::mouseReleaseEvent(event);
}


