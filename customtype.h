#ifndef CUSTOMTYPE_H
#define CUSTOMTYPE_H

#include <QtDBus/QtDBus>

#define UNUSED(x) (void)x

enum BioType {
	BIOTYPE_FINGERPRINT,
	BIOTYPE_FINGERVEIN,
	BIOTYPE_IRIS,
	__MAX_NR_BIOTYPES
};

/* 录入/删除/搜索等 D-Bus 调用的最终结果，即返回值里的 result */
enum DBusResult {
	DBUS_RESULT_SUCCESS = 0,
	DBUS_RESULT_ERROR,
	DBUS_RESULT_DEVICEBUSY,
	DBUS_RESULT_NOSUCHDEVICE,
	DBUS_RESULT_PERMISSIONDENIED
};

/* 设备操作结果 ops_status，由 UpdateStatus 函数获得 */
enum OpsStatus {
	OPS_SUCCESS,
	OPS_FAILED,
	OPS_ERROR,
	OPS_CANCEL,
	OPS_TIMEOUT,
	__MAX_NR_OPSCODES
};

struct DeviceInfo {
	int device_id;
	QString device_shortname; /* aka driverName */
	QString device_fullname;
	int enable;
	int biotype;
	int stotype;
	int eigtype;
	int vertype;
	int idtype;
	int bustype;
	int dev_status;
	int ops_status;
};

struct BiometricInfo {
	int uid;
	int biotype;
	QString device_shortname;
	int index;
	QString index_name;
};

/* StatusChanged D-Bus 信号触发时的状态变化类型 */
enum StatusType {
	STATUS_DEVICE,
	STATUS_OPERATION,
	STATUS_NOTIFY
};

Q_DECLARE_METATYPE(DeviceInfo)
Q_DECLARE_METATYPE(BiometricInfo)
Q_DECLARE_METATYPE(QList<QDBusVariant>)

void registerCustomTypes();
QDBusArgument &operator<<(QDBusArgument &argument, const DeviceInfo &deviceInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, DeviceInfo &deviceInfo);
QDBusArgument &operator<<(QDBusArgument &argument, const BiometricInfo &biometricInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, BiometricInfo &biometricInfo);

#endif // CUSTOMTYPE_H
