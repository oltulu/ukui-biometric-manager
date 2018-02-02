#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include "customtype.h"
#include <QFile>
#include <QProcessEnvironment>
#include "contentpane.h"

MainWindow::MainWindow(QString usernameFromCmd, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	usernameFromCmd(usernameFromCmd)
{
	ui->setupUi(this);
	/* 设置窗口图标 */
	QApplication::setWindowIcon(QIcon(":/images/assets/icon.png"));
	/* 设置 CSS */
	QFile qssFile(":/css/assets/mainwindow.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	qApp->setStyleSheet(styleSheet);
	qssFile.close();

	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
				"/cn/kylinos/Biometric",
				QDBusConnection::systemBus(), this);
	biometricInterface->setTimeout(2147483647); /* 微秒 */

	/* 获取设备列表 */
	getDeviceInfo();

	/* 获取并显示用户列表 */
	showUserList();
	setDefaultUser();

	/* Other initializations */
	tabPageInit();
}

MainWindow::~MainWindow()
{
	delete ui;
}


/**
 * @brief 获取并显示用户列表
 */
void MainWindow::showUserList()
{
	QFile file("/etc/passwd");
	QString line;
	QStringList fields;
	QString uname;
	int uid;

	if(!file.open(QIODevice::ReadOnly)) {
		qDebug() << "GUI:" << "/etc/passwd can not be opened";
	}

	QTextStream in(&file);

	/* 阻止 addItem 触发 currentIndexChanged 信号 */
	ui->comboBoxUsername->blockSignals(true);
	while(!in.atEnd()) {
		line = in.readLine();
		fields = line.split(":");
		uname = fields[0];
		uid = fields[2].toInt();
		if (uid == 65534) /* nobody 用户 */
			continue;
		if (uid >=1000 || uid == 0)
			ui->comboBoxUsername->addItem(uname, QVariant(uid));
	}
	file.close();
	ui->comboBoxUsername->blockSignals(false);
}

void MainWindow::setDefaultUser()
{
	/* 设置下拉列表的当前项，触发 currentIndexChanged 信号 */
	if (usernameFromCmd == "") {
		/* 获取当前执行程序的用户名 */
		QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
		ui->comboBoxUsername->setCurrentText(environment.value("USER"));
	} else {
		ui->comboBoxUsername->setCurrentText(usernameFromCmd);
	}
}

/**
 * @brief 切换 ComboBox 的 Slot
 * @param index
 */
void MainWindow::on_comboBoxUsername_currentIndexChanged(int index)
{
	qDebug() << "GUI:" << "Username ComboBox Changed";
	int uid = ui->comboBoxUsername->itemData(index).toInt();
	emit selectedUserChanged(uid);
}


/**
 * @brief 获取设备列表并存储起来备用
 */
void MainWindow::getDeviceInfo()
{
	QVariant variant;
	QDBusArgument argument;
	QList<QDBusVariant> qlist;
	QDBusVariant item;
	DeviceInfo *deviceInfo;

	/* 返回值为 i -- int 和 av -- array of variant */
	QDBusPendingReply<int, QList<QDBusVariant> > reply = biometricInterface->GetDevList();
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		deviceCount = 0;
		return;
	}

	/* 解析 DBus 返回值，reply 有两个返回值，都是 QVariant 类型 */
	variant = reply.argumentAt(0); /* 得到第一个返回值 */
	deviceCount = variant.value<int>(); /* 解封装得到设备个数 */
	variant = reply.argumentAt(1); /* 得到第二个返回值 */
	argument = variant.value<QDBusArgument>(); /* 解封装，获取QDBusArgument对象 */
	argument >> qlist; /* 使用运算符重载提取 argument 对象里面存储的列表对象 */

	for (int i = 0; i < deviceCount; i++) {
		item = qlist[i]; /* 取出一个元素 */
		variant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		argument = variant.value<QDBusArgument>();
		deviceInfo = new DeviceInfo();
		argument >> *deviceInfo; /* 提取最终的 DeviceInfo 结构体 */
		deviceInfoList[i] = deviceInfo;
	}
}

void MainWindow::on_tabWidgetMain_currentChanged(int index)
{
	QObject *currentPage = ui->tabWidgetMain->widget(index);
	QString pageName = currentPage->objectName();
	QListWidget *lw;
	if (pageName == "pageDashboard")
		return;
	else if (pageName == "pageFingerprint")
		lw = ui->listWidgetFingerprint;
	else if (pageName == "pageFingervein")
		lw = ui->listWidgetFingervein;
	else if (pageName == "pageIris")
		lw = ui->listWidgetIris;
	if (lw->count() >= 1)
		lw->setCurrentRow(0);
}

#define widgetAppendToTabPage(biometric) do {				\
	QListWidget *lw = ui->listWidget##biometric;			\
	QString sn = deviceInfoList[i]->device_shortname;		\
	lw->insertItem(lw->count(), sn);				\
	QStackedWidget *sw = ui->stackedWidget##biometric;		\
	ContentPane *contentPane = new ContentPane(deviceInfoList[i]);	\
	sw->addWidget(contentPane);					\
	connect(this, &MainWindow::selectedUserChanged,			\
			contentPane, &ContentPane::setSelectedUser);	\
} while(0)
void MainWindow::tabPageInit()
{
	for (int i = 0; i < deviceCount; i++) {
		switch (deviceInfoList[i]->biotype) {
		case BIOTYPE_FINGERPRINT:
			widgetAppendToTabPage(Fingerprint);
			break;
		case BIOTYPE_FINGERVEIN:
			widgetAppendToTabPage(Fingervein);
			break;
		case BIOTYPE_IRIS:
			widgetAppendToTabPage(Iris);
			break;
		}
	}
	connect(ui->listWidgetFingerprint, &QListWidget::currentRowChanged,
		this, &MainWindow::changeContentPane);
	connect(ui->listWidgetFingervein, &QListWidget::currentRowChanged,
		this, &MainWindow::changeContentPane);
	connect(ui->listWidgetIris, &QListWidget::currentRowChanged,
		this, &MainWindow::changeContentPane);
}

void MainWindow::changeContentPane(int index)
{
	QObject *senderObject = sender();
	QString senderName = senderObject->objectName();
	QStackedWidget *sw;
	if (senderName == "listWidgetFingerprint")
		sw = ui->stackedWidgetFingerprint;
	else if (senderName == "listWidgetFingervein")
		sw = ui->stackedWidgetFingervein;
	else if (senderName == "listWidgetIris")
		sw = ui->stackedWidgetIris;
	qDebug() << "GUI:" << "ContentPane Changed by" << senderName;
	/* 切换 ContentPane 并显示数据 */
	sw->setCurrentIndex(index);
	ContentPane *currentContentPane = (ContentPane *)sw->widget(index);
	currentContentPane->showBiometrics();
}
