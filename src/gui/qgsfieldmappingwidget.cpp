/***************************************************************************
  qgsfieldmappingwidget.cpp - QgsFieldMappingWidget

 ---------------------
 begin                : 16.3.2020
 copyright            : (C) 2020 by Alessandro Pasotti
 email                : elpaso at itopen dot it
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsfieldmappingwidget.h"
#include "qgsfieldexpressionwidget.h"
#include "qgsexpression.h"

#ifdef ENABLE_MODELTEST
#include "modeltest.h"
#endif

QgsFieldMappingWidget::QgsFieldMappingWidget( QWidget *parent,
    const QgsFields &sourceFields,
    const QgsFields &destinationFields,
    const QMap<QString, QString> &expressions )
  : QWidget( parent )
{

  setupUi( this );

  mModel = new QgsFieldMappingModel( sourceFields, destinationFields, expressions, this );

#ifdef ENABLE_MODELTEST
  new ModelTest( mModel, this );
#endif

  mTableView->setModel( mModel );
  mTableView->setItemDelegateForColumn( static_cast<int>( QgsFieldMappingModel::ColumnDataIndex::SourceExpression ), new ExpressionDelegate( mTableView ) );
  mTableView->setItemDelegateForColumn( static_cast<int>( QgsFieldMappingModel::ColumnDataIndex::DestinationType ), new TypeDelegate( mTableView ) );
  updateColumns();
  // Make sure columns are updated when rows are added
  connect( mModel, &QgsFieldMappingModel::rowsInserted, this, [ = ] { updateColumns(); } );
  connect( mModel, &QgsFieldMappingModel::modelReset, this, [ = ] { updateColumns(); } );
}

void QgsFieldMappingWidget::setDestinationEditable( bool editable )
{
  qobject_cast<QgsFieldMappingModel *>( mModel )->setDestinationEditable( editable );
  updateColumns();
}

bool QgsFieldMappingWidget::destinationEditable() const
{
  return qobject_cast<QgsFieldMappingModel *>( mModel )->destinationEditable();
}

QgsFieldMappingModel *QgsFieldMappingWidget::model() const
{
  return qobject_cast<QgsFieldMappingModel *>( mModel );
}

QList<QgsFieldMappingModel::Field> QgsFieldMappingWidget::mapping() const
{
  return model()->mapping();
}

QItemSelectionModel *QgsFieldMappingWidget::selectionModel()
{
  return mTableView->selectionModel();
}

void QgsFieldMappingWidget::setSourceFields( const QgsFields &sourceFields )
{
  model()->setSourceFields( sourceFields );
}

void QgsFieldMappingWidget::setDestinationFields( const QgsFields &destinationFields, const QMap<QString, QString> &expressions )
{
  model()->setDestinationFields( destinationFields, expressions );
}

void QgsFieldMappingWidget::scrollTo( const QModelIndex &index ) const
{
  mTableView->scrollTo( index );
}

void QgsFieldMappingWidget::appendField( const QgsField &field, const QString &expression )
{
  model()->appendField( field, expression );
}

bool QgsFieldMappingWidget::removeSelectedFields()
{
  if ( ! mTableView->selectionModel()->hasSelection() )
    return false;

  std::list<int> rowsToRemove { selectedRows() };
  rowsToRemove.reverse();
  for ( int row : rowsToRemove )
  {
    if ( ! model()->removeField( model()->index( row, 0, QModelIndex() ) ) )
    {
      return false;
    }
  }
  return true;
}

bool QgsFieldMappingWidget::moveSelectedFieldsUp()
{
  if ( ! mTableView->selectionModel()->hasSelection() )
    return false;

  const std::list<int> rowsToMoveUp { selectedRows() };
  for ( int row : rowsToMoveUp )
  {
    if ( ! model()->moveUp( model()->index( row, 0, QModelIndex() ) ) )
    {
      return false;
    }
  }
  return true;
}

bool QgsFieldMappingWidget::moveSelectedFieldsDown()
{
  if ( ! mTableView->selectionModel()->hasSelection() )
    return false;

  std::list<int> rowsToMoveDown { selectedRows() };
  rowsToMoveDown.reverse();
  for ( int row : rowsToMoveDown )
  {
    if ( ! model()->moveDown( model()->index( row, 0, QModelIndex() ) ) )
    {
      return false;
    }
  }
  return true;
}

void QgsFieldMappingWidget::updateColumns()
{
  for ( int i = 0; i < mModel->rowCount(); ++i )
  {
    mTableView->openPersistentEditor( mModel->index( i, static_cast<int>( QgsFieldMappingModel::ColumnDataIndex::SourceExpression ) ) );
    mTableView->openPersistentEditor( mModel->index( i, static_cast<int>( QgsFieldMappingModel::ColumnDataIndex::DestinationType ) ) );
  }

  for ( int i = 0; i < mModel->columnCount(); ++i )
  {
    mTableView->resizeColumnToContents( i );
  }
}

std::list<int> QgsFieldMappingWidget::selectedRows()
{
  std::list<int> rows;
  if ( mTableView->selectionModel()->hasSelection() )
  {
    const QModelIndexList constSelection { mTableView->selectionModel()->selectedIndexes() };
    for ( const QModelIndex &index : constSelection )
    {
      rows.push_back( index.row() );
    }
    rows.sort();
    rows.unique();
  }
  return rows;
}

void QgsFieldMappingWidget::ExpressionDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  QgsFieldExpressionWidget *editorWidget { qobject_cast<QgsFieldExpressionWidget *>( editor ) };
  if ( ! editorWidget )
    return;

  bool isExpression;
  bool isValid;
  const QString currentValue { editorWidget->currentField( &isExpression, &isValid ) };
  if ( isExpression )
  {
    model->setData( index, currentValue, Qt::EditRole );
  }
  else
  {
    model->setData( index, QgsExpression::quotedColumnRef( currentValue ), Qt::EditRole );
  }
}

void QgsFieldMappingWidget::ExpressionDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
  QgsFieldExpressionWidget *editorWidget { qobject_cast<QgsFieldExpressionWidget *>( editor ) };
  if ( ! editorWidget )
    return;

  const QVariant value = index.model()->data( index, Qt::EditRole );
  editorWidget->setField( value.toString() );
}

QWidget *QgsFieldMappingWidget::ExpressionDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  Q_UNUSED( option )
  QgsFieldExpressionWidget *editor = new QgsFieldExpressionWidget( parent );
  editor->setAutoFillBackground( true );
  editor->setAllowEvalErrors( false );
  editor->setAllowEmptyFieldName( true );
  const QgsFieldMappingModel *model { qobject_cast<const QgsFieldMappingModel *>( index.model() ) };
  Q_ASSERT( model );
  editor->registerExpressionContextGenerator( model->contextGenerator() );
  editor->setFields( model->sourceFields() );
  editor->setField( index.model()->data( index, Qt::DisplayRole ).toString() );
  connect( editor,
           qgis::overload<const  QString &, bool >::of( &QgsFieldExpressionWidget::fieldChanged ),
           this,
           [ = ]( const QString & fieldName, bool isValid )
  {
    Q_UNUSED( fieldName )
    Q_UNUSED( isValid )
    const_cast< QgsFieldMappingWidget::ExpressionDelegate *>( this )->emit commitData( editor );
  } );
  return editor;
}

QgsFieldMappingWidget::ExpressionDelegate::ExpressionDelegate( QObject *parent )
  : QStyledItemDelegate( parent )
{
}

QgsFieldMappingWidget::TypeDelegate::TypeDelegate( QObject *parent )
  : QStyledItemDelegate( parent )
{
}

QWidget *QgsFieldMappingWidget::TypeDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  Q_UNUSED( option )
  QComboBox *editor = new QComboBox( parent );
  const QgsFieldMappingModel *model { qobject_cast<const QgsFieldMappingModel *>( index.model() ) };
  Q_ASSERT( model );
  const QMap<QVariant::Type, QString> typeList { model->dataTypes() };
  int i = 0;
  for ( auto it = typeList.constBegin(); it != typeList.constEnd(); ++it )
  {
    editor->addItem( typeList[ it.key() ] );
    editor->setItemData( i, static_cast<int>( it.key() ), Qt::UserRole );
    ++i;
  }
  if ( ! model->destinationEditable() )
  {
    editor->setEnabled( false );
  }
  else
  {
    connect( editor,
             qgis::overload<int >::of( &QComboBox::currentIndexChanged ),
             this,
             [ = ]( int currentIndex )
    {
      Q_UNUSED( currentIndex )
      const_cast< QgsFieldMappingWidget::TypeDelegate *>( this )->emit commitData( editor );
    } );
  }
  return editor;
}

void QgsFieldMappingWidget::TypeDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
  QComboBox *editorWidget { qobject_cast<QComboBox *>( editor ) };
  if ( ! editorWidget )
    return;

  const QVariant value { index.model()->data( index, Qt::EditRole ) };
  editorWidget->setCurrentIndex( editorWidget->findData( value ) );
}

void QgsFieldMappingWidget::TypeDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
  QComboBox *editorWidget { qobject_cast<QComboBox *>( editor ) };
  if ( ! editorWidget )
    return;

  const QVariant currentValue { editorWidget->currentData( ) };
  model->setData( index, currentValue, Qt::EditRole );
}
