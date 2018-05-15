#include "monitor.h"

#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>
#include <QIcon>
#include <QRect>
#include <QLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QString>

#include "ico_ok.xpm"
#include "ico_error.xpm"

#include <QDebug>

Monitor::Monitor(QWidget *parent) : QWidget(parent)
{
    QTreeWidgetItem *item;
    //const QRect screen=QApplication::desktop()->screenGeometry();
    //this->move(screen.center()-this->rect().center());
    setWindowTitle("Мониторинг");
    tw_info = new QTreeWidget(this);
    tw_info->setRootIsDecorated(false);
    tw_info->setSortingEnabled(false);

    tw_info->setColumnCount(4);
    tw_info->header()->hide();

    item = new QTreeWidgetItem(tw_info);
    item->setText(0, "Процессор");
    item->setText(1, "Загрузка");
    tw_info->addTopLevelItem(item);
    item = new QTreeWidgetItem(tw_info);
    item->setText(0, "Память");
    item->setText(1, "Загрузка");
    item->setText(2, "Порог");
    item->setText(3, "Объем");
    tw_info->addTopLevelItem(item);
    item = new QTreeWidgetItem(tw_info);
    item->setText(0, "Диски");
    item->setText(1, "Свободно");
    item->setText(2, "Порог");
    item->setText(3, "Объем");
    tw_info->addTopLevelItem(item);


    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tw_info);


    sti_Tray = new QSystemTrayIcon(this);
    //sti_Tray->setIcon(QIcon(ico_ok_xpm));
    //QPixmap((const char**)filesave)
    sti_Tray->setIcon(QIcon(QPixmap((const char**)ico_ok_xpm)));
    sti_Tray->setObjectName("Монитор");
    sti_Tray->setVisible(true);

    if(!sti_Tray->isSystemTrayAvailable())
    qDebug()<<"sti_Tray->isSystemTrayAvailable() false";

    a_Monitor = new QAction("Мониторинг", this);
    a_Conf = new QAction("Настройка", this);
    connect(a_Monitor, SIGNAL(triggered()), SLOT(show()));
    //connect(a_Conf, SIGNAL(triggered()), rmc, SLOT(show()));
    //connect(sti_Tray, SIGNAL(messageClicked()), rm, SLOT(show()));
    //connect(sti_Tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(slotTrayActiv(QSystemTrayIcon::ActivationReason)));

    QMenu *menu = new QMenu;
    menu->addAction(a_Monitor);
    menu->addSeparator();
    menu->addAction(a_Conf);
    sti_Tray->setContextMenu(menu);
    sti_Tray->show();

    TimeCPU=5;
    LimitMem=80;
    LimitHdd=20;

    p_HDD = new QProcess(this);
    connect(p_HDD, SIGNAL(finished(int)), SLOT(hdd_info(int)));

    t_Update=new QTimer(this);
    t_Update->start(1000);
    connect(t_Update, SIGNAL(timeout()), SLOT(slotTimer()));

}
void Monitor::slotTrayActiv(QSystemTrayIcon::ActivationReason reason)
{
    if(reason==QSystemTrayIcon::DoubleClick)
        show();
}
//---------------------------------------------------------------------------------
void Monitor::slotTimer()
{
    cpu_load();
    mem_info();

    if(p_HDD->atEnd())
        p_HDD->start("/bin/df -h");

    if((ErrorCPU)||(ErrorHDD)||(ErrorMem))
        emit signalError(true);
    else
        emit signalError(false);
    tw_info->expandAll();
    tw_info->resizeColumnToContents(0);
    tw_info->resizeColumnToContents(1);
    tw_info->resizeColumnToContents(2);
    tw_info->resizeColumnToContents(3);
}
//---------------------------------------------------------------------------------
void Monitor::cpu_load()
{
    QString str=ReadInfo("/proc/stat");
    QStringList lst=str.split("\n");
    QStringList ss;
    QTreeWidgetItem *itm;
    QBrush Br (Qt::red);
    QBrush Bb (Qt::black);

    int ii=0;
    OLD_CPU tmp;

    double pct, activ, total;

    for(int j=tw_info->topLevelItem(0)->childCount()-1; j>=0; j--) // удаление дочек
        tw_info->topLevelItem(0)->removeChild(tw_info->topLevelItem(0)->child(j));

    ErrorCPU=false;
    for(int i=0; i<lst.count(); i++)
    {
        if(!lst.at(i).length()) // отброс пустых строк
            continue;
        ss=lst.at(i).simplified().split(" "); // удаление лишних пробелов и разложение по значениям

        if(ss.at(0).startsWith("cpu")) //
        {
            if(ss.at(0).length()==3)
                continue;
            if(ss.count()<5)
                continue;

            tmp.user=ss.at(1).toUInt();
            tmp.nice=ss.at(2).toUInt();
            tmp.system=ss.at(3).toUInt();
            tmp.idle=ss.at(4).toUInt();
            if(CpuStart)
            {
                OldCpu.append(tmp);
                for(int ci=0; ci<20; ci++)
                    tCPU[ci]=0;
            }
            else
            {
                activ=(tmp.user-OldCpu.at(ii).user)+(tmp.nice-OldCpu.at(ii).nice)+(tmp.system-OldCpu.at(ii).system);
                total=activ+(tmp.idle-OldCpu.at(ii).idle);
                pct=(100*activ)/total;

                OldCpu.replace(ii, tmp);

                itm = new QTreeWidgetItem(tw_info->topLevelItem(0));
                itm->setText(0, QString("Ядро %1").arg(ss.at(0).mid(3).toInt()+1));
                itm->setText(1, QString("%1 %").arg((int)pct));

                if(pct>=100)
                {
                    itm->setForeground(0, Br);
                    itm->setForeground(1, Br);
                    if(tCPU[ii]>=(TimeCPU*60))
                        ErrorCPU=true;
                    else
                        tCPU[ii]++;
                }
                else
                {
                    for(int ci=0; ci<20; ci++)
                        tCPU[ci]=0;
                    itm->setForeground(0, Bb);
                    itm->setForeground(1, Bb);
                }
                tw_info->topLevelItem(0)->addChild(itm);
            }
            ii++;
        }
    }
    if(CpuStart)
        CpuStart=false;
}
//---------------------------------------------------------------------------------
void Monitor::mem_info()
{
    QTreeWidgetItem *itm;
    QBrush Br (Qt::red);
    QBrush Bb (Qt::black);

    QString str=ReadInfo("/proc/meminfo");
    QStringList lst=str.split("\n");
    QStringList ss;

    int mem, swap;
    double mem_load, mem_total;
    double swap_load, swap_total;

    for(int j=tw_info->topLevelItem(1)->childCount()-1; j>=0; j--) // удаление дочек
        tw_info->topLevelItem(1)->removeChild(tw_info->topLevelItem(1)->child(j));
    for(int i=0; i<lst.count(); i++)
    {
        if(!lst.at(i).length()) // отброс пустых строк
            continue;
        ss=lst.at(i).simplified().split(" "); // удаление лишних пробелов и разложение по значениям
        if(ss.at(0).startsWith("MemTotal"))
        {
            mem_total=ss.at(1).toDouble()/1024;
        }
        else if (ss.at(0).startsWith("MemFree"))
        {
            mem_load=ss.at(1).toDouble()/1024;
        }
        else if (ss.at(0).startsWith("Cached"))
        {
            mem_load+=ss.at(1).toDouble()/1024;
        }
        else if (ss.at(0).startsWith("SwapTotal"))
        {
            swap_total=ss.at(1).toDouble()/1024;
        }
        else if (ss.at(0).startsWith("SwapFree"))
        {
            swap_load=ss.at(1).toDouble()/1024;
        }
    }

    itm = new QTreeWidgetItem(tw_info->topLevelItem(1));
    itm->setText(0, "ОЗУ");
    mem_load=mem_total-mem_load;
    swap_load=swap_total-swap_load;

    mem=(int)((100/mem_total)*mem_load);
    swap=(int)((100/swap_total)*swap_load);

    if(mem>=LimitMem)
    {
        itm->setForeground(0, Br);
        itm->setForeground(1, Br);
        ErrorMem=true;
    }
    else
    {
        itm->setForeground(0, Bb);
        itm->setForeground(1, Bb);
    }
    itm->setText(1, QString("%1 %").arg(mem));
    itm->setText(2, QString("%1 %").arg(LimitMem));
    itm->setText(3, QString("%1M").arg(mem_total));
    tw_info->topLevelItem(1)->addChild(itm);

    itm = new QTreeWidgetItem(tw_info->topLevelItem(1));
    itm->setText(0, "swap");
    if(swap>=LimitMem)
    {
        itm->setForeground(0, Br);
        itm->setForeground(1, Br);
        ErrorMem=true;
    }
    else
    {
        itm->setForeground(0, Bb);
        itm->setForeground(1, Bb);
    }
    itm->setText(1, QString("%1 %").arg(swap));
    itm->setText(2, QString("%1 %").arg(LimitMem));
    itm->setText(3, QString("%1M").arg(swap_total));
    tw_info->topLevelItem(1)->addChild(itm);

}
//---------------------------------------------------------------------------------
void Monitor::hdd_info(int s)
{
    QByteArray contents = p_HDD->readAll();
    QTextStream in(&contents);

    int freehdd;
    ErrorHDD=false;
    QTreeWidgetItem *itm;
    QBrush Br (Qt::red);
    QBrush Bb (Qt::black);
    QStringList ss, lst= in.readAll().split("\n");

    for(int j=tw_info->topLevelItem(2)->childCount()-1; j>=0; j--) // удаление дочек
        tw_info->topLevelItem(2)->removeChild(tw_info->topLevelItem(2)->child(j));

    for(int i=1; i<lst.count(); i++)
    {
        if(!lst.at(i).length())
            continue;
        ss=lst.at(i).split(QRegExp("\\s+"));

        itm = new QTreeWidgetItem(tw_info->topLevelItem(2));

        freehdd=100-ss.at(4).left(ss.at(4).length()-1).toInt();

        itm->setText(0, ss.at(0));
        itm->setText(1, QString("%1 %").arg(freehdd));
        itm->setText(2, QString("%1 %").arg(LimitHdd));
        itm->setText(3, ss.at(1));

        if(freehdd<=LimitHdd)
        {
            itm->setForeground(0, Br);
            itm->setForeground(1, Br);
            ErrorHDD=true;
        }
        else
        {
            itm->setForeground(0, Bb);
            itm->setForeground(1, Bb);
        }

        tw_info->topLevelItem(2)->addChild(itm);
    }
}
//---------------------------------------------------------------------------------
QString Monitor::ReadInfo(QString fileName)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
    {
        qCritical()<<"Unable to open file "+fileName+", aborting";
        return "";
    }
    if (!file.isReadable())
    {
        qCritical()<<"Unable to read file "+fileName+", aborting";
        return "";
    }
    QByteArray contents = file.readAll();
    QTextStream in(&contents);
    QString line=in.readAll();
    file.close();
    return line;

}
