/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef CONFIGURATIONS2003WIDGET_H
#define CONFIGURATIONS2003WIDGET_H

#include "vcnodewidget.h"
#include "../vcprojectmodel/configuration.h"

#include <QList>
#include <QMap>

namespace VcProjectManager {
namespace Internal {

class Configurations;
class Configuration;
class VcProjectDocument;
class ConfigurationsWidget;
class IFile;
class IFileContainer;

class ConfigurationsBaseWidget : public VcNodeWidget
{
    Q_OBJECT

public:
    explicit ConfigurationsBaseWidget(Configurations *configs, VcProjectDocument *vcProjDoc);
    ~ConfigurationsBaseWidget();
    void saveData();

private slots:
    void onAddNewConfig(QString newConfigName, QString copyFrom);
    void onRenameConfig(QString newConfigName, QString oldConfigNameWithPlatform);
    void onRemoveConfig(QString configNameWithPlatform);

protected:
    void addConfiguration(IConfiguration *config);
    void removeConfiguration(IConfiguration *config);
    IConfiguration* createConfiguration(const QString &configNameWithPlatform) const;
    IConfiguration* configInNewConfigurations(const QString &configNameWithPlatform) const;
    void addConfigurationToFiles(const QString &copyFromConfig, const QString &targetConfigName);
    void addConfigurationToFilesInFilter(IFileContainer *filterPtr, const QString &copyFromConfig, const QString &targetConfigName);
    void addConfigurationToFile(IFile *filePtr, const QString &copyFromConfig, const QString &targetConfigName);

    Configurations *m_configs;
    VcProjectDocument *m_vcProjDoc;
    ConfigurationsWidget *m_configsWidget;

    QList<IConfiguration *> m_newConfigurations;
    QList<QString> m_removedConfigurations;
    QMap<IConfiguration*, QString> m_renamedConfigurations; // <oldName, newName>

    QHash<IFile*, QList<IConfiguration*> > m_newFilesConfigurations;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // CONFIGURATIONS2003WIDGET_H
