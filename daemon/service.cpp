#include "service.h"
#include "datasourcecpu.h"
#include "datasourcebattery.h"
#include "datasourcecell.h"
#include "datasourcewlan.h"

Service::Service(QObject *parent, Settings *settings) :
    QObject(parent), m_settings(settings)
{
    initDataSources();

    connect(m_settings, SIGNAL(updateIntervalChanged(int)), SLOT(updateIntervalChanged(int)));

    m_background = new BackgroundActivity(this);
    connect(m_background, SIGNAL(running()), SLOT(backgroundRunning()));
    m_background->setWakeupFrequency(BackgroundActivity::Range);

    /*
    connect(&m_timer,SIGNAL(timeout()), SLOT(gatherData()));
    m_timer.setTimerType(Qt::VeryCoarseTimer);
    m_timer.setInterval(1000);
    m_timer.start();
    */
    dbus = new DBusAdapter(this);

    updateIntervalChanged(m_settings->updateInterval);
}

void Service::initDataSources() {
    m_sources.append(new DataSourceCPU(this));
    m_sources.append(new DataSourceBattery(this));
    m_sources.append(new DataSourceWlan(this));
    m_sources.append(new DataSourceCell(this));
    //m_sources.append(new DataSourceTemp(this));

    foreach(const DataSource* source, m_sources) {
        connect(source, SIGNAL(dataGathered(DataSource::Type,float)), SLOT(dataGathered(DataSource::Type,float)));
    }
}

void Service::backgroundRunning()
{
    gatherData();
    m_background->wait();
}

void Service::updateIntervalChanged(int interval) {
    qDebug() << "Update interval" << interval;
    m_background->setWakeupRange(interval, interval);
    /*
    if (m_timer.interval() != interval*1000) {
        m_timer.setInterval(interval*1000);
    }
    */
    if (m_background->state() != BackgroundActivity::Running) {
        m_background->run();
    }
}

void Service::gatherData() {
    qDebug() << "Gather data start";
    m_updateTime = QDateTime::currentDateTimeUtc();
    foreach (DataSource *source, m_sources) {
        source->gatherData();
    }
    removeObsoleteData();
    qDebug() << "Gather data end";
    emit dataUpdated();
}

void Service::removeObsoleteData() {
    m_storage.removeObsoleteData(m_updateTime.addDays(-1 * m_settings->updateInterval));
}


void Service::dataGathered(DataSource::Type type, float value) {
    qDebug() << "DataGathered" << type << value;
    m_storage.save(type, m_updateTime, value);
}