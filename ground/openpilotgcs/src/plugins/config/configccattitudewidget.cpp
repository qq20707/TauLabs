/**
 ******************************************************************************
 *
 * @file       configccattitudewidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Configure Attitude module on CopterControl
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include "configccattitudewidget.h"
#include "ui_ccattitude.h"
#include "utils/coordinateconversions.h"
#include <QMutexLocker>
#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include "accels.h"
#include "gyros.h"
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/generalsettings.h>
#include "homelocation.h"
#include "attitudesettings.h"
#include "inertialsensorsettings.h"

// *****************

int SixPointInConstFieldCal2( double ConstMag, double x[6], double y[6], double z[6], double S[3], double b[3] );
int LinearEquationsSolving2(int nDim, double* pfMatr, double* pfVect, double* pfSolution);


class Thread : public QThread
{
public:
    static void usleep(unsigned long usecs)
    {
        QThread::usleep(usecs);
    }
};

#define sign(x) ((x < 0) ? -1 : 1)

ConfigCCAttitudeWidget::ConfigCCAttitudeWidget(QWidget *parent) :
        ConfigTaskWidget(parent),
        ui(new Ui_ccattitude)
{
    ui->setupUi(this);
    forceConnectedState(); //dynamic widgets don't recieve the connected signal

    ExtensionSystem::PluginManager *pm=ExtensionSystem::PluginManager::instance();
    Core::Internal::GeneralSettings * settings=pm->getObject<Core::Internal::GeneralSettings>();
    if(!settings->useExpertMode())
        ui->applyButton->setVisible(false);
    
    addApplySaveButtons(ui->applyButton,ui->saveButton);
    addUAVObject("AttitudeSettings");
    addUAVObject("InertialSensorSettings");
    addUAVObject("HwSettings");

    // Load UAVObjects to widget relations from UI file
    // using objrelation dynamic property
    autoLoadWidgets();
    addUAVObjectToWidgetRelation("AttitudeSettings","ZeroDuringArming",ui->zeroGyroBiasOnArming);

    // Connect signals
    connect(ui->ccAttitudeHelp, SIGNAL(clicked()), this, SLOT(openHelp()));
    connect(ui->sixPointsStart, SIGNAL(clicked()), &calibration, SLOT(doStartSixPoint()));
    connect(ui->sixPointsSave, SIGNAL(clicked()),  &calibration, SLOT(doSaveSixPointPosition()));
    connect(ui->levelButton ,SIGNAL(clicked()), &calibration, SLOT(doStartLeveling()));

    // Connect feedback from calibration
    connect(&calibration, SIGNAL(progressChanged(int)), ui->levelProgress, SLOT(setValue(int)));
    connect(&calibration, SIGNAL(showMessage(QString)), ui->calibrationMessages, SLOT(setText(QString)));
    connect(&calibration, SIGNAL(toggleControls(bool)), this, SLOT(enableControls(bool)));

    addWidget(ui->levelButton);
    refreshWidgetsValues();
}

ConfigCCAttitudeWidget::~ConfigCCAttitudeWidget()
{
    delete ui;
}

void ConfigCCAttitudeWidget::openHelp()
{

    QDesktopServices::openUrl( QUrl("http://wiki.openpilot.org/x/44Cf", QUrl::StrictMode) );
}

void ConfigCCAttitudeWidget::enableControls(bool enable)
{
    if(ui->levelButton)
        ui->levelButton->setEnabled(enable);
    ConfigTaskWidget::enableControls(enable);
}

void ConfigCCAttitudeWidget::updateObjectsFromWidgets()
{
    ConfigTaskWidget::updateObjectsFromWidgets();

    ui->levelProgress->setValue(0);
}

/**
 * @}
 * @}
 */
