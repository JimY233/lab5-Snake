

/* Necessary includes for device drivers */
#include <asm/system_misc.h> /* cli(), *_flags */
#include <asm/uaccess.h>     /* copy_from/to_user */
#include <linux/errno.h>     /* error codes */
#include <linux/fcntl.h>     /* O_ACCMODE */
#include <linux/fs.h>        /* everything... */
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h> /* printk() */
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/timer.h>
#include <linux/types.h> /* size_t */
#include <linux/uaccess.h>

#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/seq_file.h>
#include <linux/sched/signal.h>


MODULE_LICENSE("Dual BSD/GPL");

static int ktimer_fasync(int fd, struct file *filp, int mode);
struct fasync_struct *async_queue; /* asynchronous readers */

#define MAX_COOKIE_LENGTH       PAGE_SIZE
static struct proc_dir_entry *proc_entry;

static char *cookie_pot;  // Space for fortune strings
static int cookie_index;  // Index to write next fortune
static int next_fortune;  // Index to read next fortune

//static ssize_t fortune_write(struct file*, const char __user*, size_t, loff_t*);
static int ktimer_proc_open(struct inode*, struct file*);
static int ktimer_proc_show(struct seq_file*, void*);

/* Declaration of memory.c functions */
static int ktimer_open(struct inode *inode, struct file *filp);
static int ktimer_release(struct inode *inode, struct file *filp);
static ssize_t ktimer_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos);
static ssize_t ktimer_write(struct file *filp, const char *buf, size_t count,
                            loff_t *f_pos);
static void ktimer_exit(void);
static int ktimer_init(void);

#define from_timer(var, callback_timer, timer_fieldname) \
  container_of(callback_timer, typeof(*var), timer_fieldname)

/* Structure that declares the usual file */
/* access functions */
struct file_operations ktimer_fops = {
 owner:    THIS_MODULE,
  read:    ktimer_read,
  //read:     seq_read,
  write:   ktimer_write,
  open:    ktimer_open,
  release: ktimer_release,
  fasync:  ktimer_fasync,
  //llseek:  seq_lseek
};

static const struct file_operations fortune_proc_fops = {
    .owner = THIS_MODULE,
    .open = ktimer_proc_open,
    .read = seq_read,
    //.write = fortune_write,
    .llseek = seq_lseek,
    .release = single_release,
};

/* Declaration of the init and exit functions */
module_init(ktimer_init);
module_exit(ktimer_exit);

static unsigned capacity = 256;
// static unsigned bite = 128;
module_param(capacity, uint, S_IRUGO);

/* Global variables of the driver */
/* Major number */
static int ktimer_major = 61;

/* Buffer to store data */
static char *ktimer_buffer;
/* length of the current message */
static int ktimer_len;

unsigned long seconds = 1;
unsigned long loadtime;

static char flag[2];

//static int num_timers = 1;

static char MSG[256];
static struct timer_list timerlist;
/*
struct timer_data {
  struct timer_list timer;
  char timer_msg[256]; // kmalloc ptr
  int active;
};

struct timer_data *tmd;
*/
static int activeflag = 0;
static int PID;
static char COMM[256];

void timer_function(struct timer_list *timer) {
  //printk("%s\n", MSG);
  //printk("%s\n", tmd[0].timer_msg);
  activeflag = 0;

  if (async_queue)
    kill_fasync(&async_queue, SIGIO, POLL_IN);

  del_timer(&timerlist);
}

static int ktimer_init(void) {
  int result;
  int ret = 0;

  /* Registering device */
  result = register_chrdev(ktimer_major, "ktimer", &ktimer_fops);
  if (result < 0) {
    printk(KERN_ALERT "ktimer: cannot obtain major number %d\n", ktimer_major);
    return result;
  }
  //tmd = kmalloc(sizeof(struct timer_data)*10, GFP_KERNEL); // create 5 tmds

  /* Allocating ktimer for the buffer */
  ktimer_buffer = kmalloc(capacity, GFP_KERNEL); //capacity*10 for tmd
  if (!ktimer_buffer) {
    printk(KERN_ALERT "Insufficient kernel memory\n");
    result = -ENOMEM;
    goto fail;
  }
  memset(ktimer_buffer, 0, capacity);
  ktimer_len = 0;

  cookie_pot = (char *)vmalloc(MAX_COOKIE_LENGTH);

  if (!cookie_pot) {
    printk(KERN_ALERT "Insufficient kernel memory\n");
    ret = -ENOMEM;
  } else {
    memset(cookie_pot, 0, MAX_COOKIE_LENGTH);
    proc_entry = proc_create("mytimer", 0644, NULL, &fortune_proc_fops);
    if (proc_entry == NULL) {
      ret = -ENOMEM;
      vfree(cookie_pot);
      printk(KERN_INFO "mytimer: Couldn't create proc entry\n");
    } else {
      cookie_index = 0;
      next_fortune = 0;
      //printk(KERN_INFO "fortune: Module loaded.\n");
    }
  }
  
  loadtime = jiffies/HZ;
  printk(KERN_ALERT "mytimer module loaded\n");
  return 0;

fail:
  ktimer_exit();
  return result;
}

static void ktimer_exit(void) {

  remove_proc_entry("mytimer", NULL);
  vfree(cookie_pot);
  //printk(KERN_INFO "fortune: Module unloaded.\n");

  /* Freeing the major number */
  unregister_chrdev(ktimer_major, "ktimer");

  /* Freeing buffer memory */
  if (ktimer_buffer) {
    kfree(ktimer_buffer);
  }
  /*
  if (tmd) {
    kfree(tmd);
  }
  */
  printk(KERN_ALERT "Removing ktimer module\n");
}

static int ktimer_open(struct inode *inode, struct file *filp) {
  // printk(KERN_INFO "open called: process id %d, command %s\n", current->pid,current->comm);
  /* Success */
  return 0;
}

static int ktimer_release(struct inode *inode, struct file *filp) {
  // printk(KERN_INFO "release called: process id %d, command %s\n", current->pid,current->comm);
  /* Success */
  ktimer_fasync(-1, filp, 0);
  return 0;
}


static ssize_t ktimer_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos) {
  unsigned long int time_left = 0;
  //int i;
  char *tbptr = kmalloc(capacity, GFP_KERNEL);
  // char fbuf[256], *fbptr =fbuf;
  //char *fbptr = kmalloc(capacity*10, GFP_KERNEL);

  // Prevents reading more than needed
  ssize_t bytes = count < (capacity -(*f_pos)) ? count : (capacity - (*f_pos));
  //memset(fbptr, 0, capacity*10);

  if (activeflag==0) {
    return 0;
  }

  time_left = (timerlist.expires - jiffies)/HZ;
  sprintf(tbptr, "%s %lu\n", MSG, time_left);
  //strcat(fbptr, tbptr);

  //strcpy(ktimer_buffer, fbptr);
  strcpy(ktimer_buffer, tbptr);

  if (copy_to_user(buf, ktimer_buffer, bytes)) {
    return -EFAULT;
  }
  //kfree(fbptr);
  kfree(tbptr);
  (*f_pos) += bytes;
  return bytes;
}


static ssize_t ktimer_write(struct file *filp, const char *buf, size_t count,
                            loff_t *f_pos) {

  int temp, i;
  //int modded = 0, added = 0;
  //int num_active_timers = 0;
  char tbuf[256], *tbptr = tbuf;
  char tset[256], *timerset = tset;
  //char *cp_buf = kmalloc(sizeof(char), GFP_KERNEL);
  char *message;
  char *sec;
  char *flag_start;
  
  if (*f_pos >= capacity) {
    return -ENOSPC;
  }
  if (count > capacity - *f_pos) count = capacity - *f_pos;
  if (copy_from_user(ktimer_buffer + *f_pos, buf, count)) {
    return -EFAULT;
  }

  for (temp = *f_pos; temp < count + *f_pos; temp++)
    tbptr += sprintf(tbptr, "%c", ktimer_buffer[temp]);

  *f_pos += count;
  ktimer_len = *f_pos;

  //strcpy(cp_buf, tbuf);
  strcpy(timerset, tbuf);

  flag_start = strsep(&timerset, ";");
  for (i = 0; i < 2; i++) {
    flag[i] = flag_start[i];
  }

  if(strcmp(flag,"-r")==0){
    if(activeflag == 1){
      del_timer(&timerlist);
      activeflag = 0;
      memset(&MSG[0],0,sizeof(MSG));
      kill_pid(find_vpid(PID),SIGTERM,1);
    }
    return count;
  }

  message = strsep(&timerset, ";"); // the message

  sec = strsep(&timerset, " "); // the time
  sscanf(sec, "%lu", &seconds);

  if(activeflag == 0){
    timer_setup(&timerlist, timer_function, 0);
    mod_timer(&timerlist, jiffies + seconds*HZ);
    PID = current->pid;
    strcpy(COMM,current->comm);
    //strcpy(tmd[0].timer_msg,message);
    strcpy(MSG,message);
    activeflag = 1;
  }
  else{
    if (strcmp(MSG, message) == 0) {
      mod_timer(&timerlist, jiffies + seconds*HZ);
      PID = current->pid;
      strcpy(COMM,current->comm);
      return -EBUSY;
    } 
    else{
      //printk("A timer exists already");
      return -EEXIST;
    }
  }

  //kfree(cp_buf);
  return count;
 
}

static int ktimer_fasync(int fd, struct file *filp, int mode) {
	return fasync_helper(fd, filp, mode, &async_queue);
}


static int ktimer_proc_show(struct seq_file *m, void *v) {
    /* Wrap-around */
    if (next_fortune >= cookie_index) {
        next_fortune = 0;
    }

    /* Use seq_files to paginate the output (just in case it's very large) */
    //seq_printf(m, "%s\n", &(cookie_pot[next_fortune]));
    seq_printf(m,"module name: %s\n", THIS_MODULE->name);
    seq_printf(m,"time since loaded: %lu\n",((jiffies/HZ)-loadtime));
    if(activeflag == 1){
      seq_printf(m,"process id: %d\tcommand:%s\texpiration time: %lu\n",PID,COMM,((timerlist.expires - jiffies)/HZ));
    }

    next_fortune += strlen(&(cookie_pot[next_fortune])) + 1;
    return 0;
}

static int ktimer_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, ktimer_proc_show, NULL);
}

/*
static ssize_t fortune_write(struct file *filp, const char __user *buff, size_t len, loff_t *fpos)
{
    int space_available = (MAX_COOKIE_LENGTH - cookie_index) + 1;
    if (len > space_available) {
        printk(KERN_INFO "fortune: cookie pot is full!\n");
        return -ENOSPC;
    }

    if (copy_from_user(&(cookie_pot[cookie_index]), buff, len)) {
        return -EFAULT;
    }

    cookie_index += len;
    cookie_pot[cookie_index-1] = 0;
    *fpos = len;
    return len;
}
*/
