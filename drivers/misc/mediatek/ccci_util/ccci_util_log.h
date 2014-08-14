#ifndef __CCCI_UTIL_LOG_H__
#define __CCCI_UTIL_LOG_H__

// No MD id message part
#define CCCI_UTIL_DBG_MSG(fmt, args...) \
do { \
	printk(KERN_DEBUG "[ccci/util](0)" fmt, ##args); \
} while(0)

#define CCCI_UTIL_INF_MSG(fmt, args...) \
do { \
	printk(KERN_NOTICE "[ccci/util](0)" fmt, ##args); \
} while(0)

#define CCCI_UTIL_ERR_MSG(fmt, args...) \
do { \
	printk(KERN_ERR "[ccci/util](0)" fmt, ##args); \
} while(0)

// With MD id message part
#define CCCI_UTIL_DBG_MSG_WITH_ID(id, fmt, args...) \
do { \
	printk(KERN_DEBUG "[ccci/util](%d)" fmt, (id+1), ##args); \
} while(0)

#define CCCI_UTIL_INF_MSG_WITH_ID(id, fmt, args...) \
do { \
	printk(KERN_NOTICE "[ccci/util](%d)" fmt, (id+1), ##args); \
} while(0)

#define CCCI_UTIL_ERR_MSG_WITH_ID(id, fmt, args...) \
do { \
	printk(KERN_ERR "[ccci/util](%d)" fmt, (id+1), ##args); \
} while(0)


#endif //__CCCI_UTIL_LOG_H__
