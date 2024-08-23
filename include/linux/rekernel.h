#include <linux/init.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/proc_fs.h>

#define NETLINK_REKERNEL_MAX     		26
#define NETLINK_REKERNEL_MIN     		22
#define USER_PORT        			100
#define PACKET_SIZE 				128
#define MIN_USERAPP_UID 			(10000)
#define MAX_SYSTEM_UID  			(2000)
#define RESERVE_ORDER				17
#define WARN_AHEAD_SPACE			(1 << RESERVE_ORDER)

struct sock *rekernel_netlink = NULL;
extern struct net init_net;
int netlink_unit = NETLINK_REKERNEL_MIN;

static inline bool line_is_frozen(struct task_struct *task)
{
    return frozen(task->group_leader) || freezing(task->group_leader);
}

static int send_netlink_message(char *msg, uint16_t len) {
    struct sk_buff *skbuffer;
    struct nlmsghdr *nlhdr;

    skbuffer = nlmsg_new(len, GFP_ATOMIC);
    if (!skbuffer) {
        printk("netlink alloc failure.\n");
        return -1;
    }

    nlhdr = nlmsg_put(skbuffer, 0, 0, netlink_unit, len, 0);
    if (!nlhdr) {
        printk("nlmsg_put failaure.\n");
        nlmsg_free(skbuffer);
        return -1;
    }

    memcpy(nlmsg_data(nlhdr), msg, len);
    return netlink_unicast(rekernel_netlink, skbuffer, USER_PORT, MSG_DONTWAIT);
}

static void netlink_rcv_msg(struct sk_buff *skbuffer) { // Ignore recv msg.
}

struct netlink_kernel_cfg rekernel_cfg = { 
    .input = netlink_rcv_msg,
};

static int rekernel_unit_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", netlink_unit);
	return 0;
}

static int rekernel_unit_open(struct inode *inode, struct file *file)
{
	return single_open(file, rekernel_unit_show, NULL);
}

static const struct file_operations rekernel_unit_fops = {
	.open   = rekernel_unit_open,
	.read   = seq_read,
	.llseek   = seq_lseek,
	.release   = single_release,
	.owner   = THIS_MODULE,
};

static struct proc_dir_entry *rekernel_dir, *rekernel_unit_entry;

static int start_rekernel_server(void) {
  if (rekernel_netlink != NULL)
    return 0;
  for (netlink_unit = NETLINK_REKERNEL_MIN; netlink_unit < NETLINK_REKERNEL_MAX; netlink_unit++) {
    rekernel_netlink = (struct sock *)netlink_kernel_create(&init_net, netlink_unit, &rekernel_cfg);
    if (rekernel_netlink != NULL)
      break;
  }
  if (rekernel_netlink == NULL) {
    printk("Failed to create Re:Kernel server!\n");
    return -1;
  }
  printk("Created Re:Kernel server! NETLINK UNIT: %d\n", netlink_unit);
  rekernel_dir = proc_mkdir("rekernel", NULL);
  if (!rekernel_dir)
      printk("create /proc/rekernel failed!\n");
  else {
      char buff[32];
      sprintf(buff, "%d", netlink_unit);
      rekernel_unit_entry = proc_create(buff, 0644, rekernel_dir, &rekernel_unit_fops);
      if (!rekernel_unit_entry)
          printk("create rekernel unit failed!\n");
  }
  return 0;
}
