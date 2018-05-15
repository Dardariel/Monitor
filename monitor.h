#ifndef MONITOR_H
#define MONITOR_H

#include <QWidget>
#include <QSystemTrayIcon>
//#include <QPlatformSystemTrayIcon>
#include <QAction>
#include <QTreeWidget>
#include <QCloseEvent>
#include <QProcess>
#include <QTimer>

struct OLD_CPU
{
    double user;
    double nice;
    double system;
    double idle;
};

class Monitor : public QWidget
{
    Q_OBJECT
public:
    explicit Monitor(QWidget *parent = nullptr);

    int TimeCPU;
    int LimitMem;
    int LimitHdd;

    QTimer *t_Update; // таймер обновления информации

    QSystemTrayIcon *sti_Tray;
    QAction *a_Monitor;
    QAction *a_Conf;

    QTreeWidget *tw_info;

signals:

public slots:
    void slotTrayActiv(QSystemTrayIcon::ActivationReason reason);

    void slotTimer();
    void hdd_info(int s);


private:
    bool FlagClose;

    void cpu_load();
    void mem_info();
    void eth_info();

    QString ReadInfo(QString fileName);

    QList<OLD_CPU> OldCpu;
    bool CpuStart;

    QProcess *p_HDD;

    int tCPU[20];

    bool ErrorCPU;
    bool ErrorMem;
    bool ErrorHDD;

signals:
    void signalError(bool);

protected:
    void closeEvent(QCloseEvent *e)
    {
        //hide();
        //e->ignore();
    }

};

#endif // MONITOR_H
