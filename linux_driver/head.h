struct kobject {
	const char		*name;
	struct list_head	entry;
	struct kobject		*parent;
	struct kset		*kset;
	struct kobj_type	*ktype;
	struct sysfs_dirent	*sd;
	struct kref		kref;
	unsigned int state_initialized:1;
	unsigned int state_in_sysfs:1;
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
};

/* 
kobject的release函数并没有包含在kobject自身内，而是用struct kobj_type
*/

struct kobj_type {
	void (*release)(struct kobject *kobj);
	struct sysfs_ops *sysfs_ops;
	struct attribute **default_attrs;
};

struct kset {
	struct list_head list;
	spinlock_t list_lock;
	struct kobject kobj;
	struct kset_uevent_ops *uevent_ops; //热插拔操作
};