

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

#include <linux/fb.h>
#include <linux/mutex.h>


MODULE_LICENSE("Dual BSD/GPL");

struct fb_info *info;

struct fb_fillrect *rect1;
struct fb_fillrect *rect2;
struct fb_fillrect *rect3;
struct fb_fillrect *rect4;

/* Helper function borrowed from drivers/video/fbdev/core/fbmem.c */
static struct fb_info *get_fb_info(unsigned int idx)
{
    struct fb_info *fb_info;

    if (idx >= FB_MAX)
        return ERR_PTR(-ENODEV);

    fb_info = registered_fb[idx];
    if (fb_info)
        atomic_inc(&fb_info->count);

    return fb_info;
}

/* Declaration of memory.c functions */
static int mycounter_open(struct inode *inode, struct file *filp);
static int mycounter_release(struct inode *inode, struct file *filp);
static ssize_t mycounter_read(struct file *filp, char *buf, size_t count,
                           loff_t *f_pos);
static ssize_t mycounter_write(struct file *filp, const char *buf, size_t count,
                            loff_t *f_pos);
static void __exit mycounter_exit(void);
static int __init mycounter_init(void);
#define from_timer(var, callback_timer, timer_fieldname) \
  container_of(callback_timer, typeof(*var), timer_fieldname)

/* Structure that declares the usual file */
/* access functions */
struct file_operations mycounter_fops = {
  read:    mycounter_read,
  write:   mycounter_write,
  open:    mycounter_open,
  release: mycounter_release
};

/* Declaration of the init and exit functions */
module_init(mycounter_init);
module_exit(mycounter_exit);

static unsigned capacity = 256;
// static unsigned bite = 128;
module_param(capacity, uint, S_IRUGO);

/* Global variables of the driver */
/* Major number */
static int mycounter_major = 61;

/* Buffer to store data */
static char *mycounter_buffer;
/* length of the current message */
static int mycounter_len;

static char flag[256];

//static int num_timers = 1;

/*
struct timer_data {
  struct timer_list timer;
  char timer_msg[256]; // kmalloc ptr
  int active;
};

struct timer_data *tmd;
*/

static struct timer_list timer;

static int count = 1;
static int mode = 0; //up=0;down=1
static int state = 0;//running=0;stopped=1
static unsigned long frequency = 1; //f1
//static unsigned long seconds = 1;

//static unsigned long int time_left = 0;

void timer_function(struct timer_list *t) {
/*
  struct timer_data *tdata = from_timer(tdata, t, timer);
  char* my_message = tdata->timer_msg;
  printk("%s\n", my_message);
  memset(mycounter_buffer, 0, capacity);
  tdata->active = 0;
  mycounter_len = 0;
*/

  del_timer(&timer);
  //printk(KERN_ALERT "count:%d;mode:%d\n",count,mode);
  if(state == 0)
  {
    if(mode == 0)
    {
      count++;
      if(count>=16)
      {
        //printk(KERN_ALERT "count:%d;mode:%d\n",count,mode);
        count = 1;
      }
    }  
    else if(mode == 1)
    {
      count--;
      if(count<=0)
      {
        //printk(KERN_ALERT "count:%d;mode:%d\n",count,mode);
        count = 15;
      }
    } 
    
  }
   
  if(count < 8)
  {
    rect1->color = 0x01;
  } 
  else
  {
    rect1->color = 0x04;
  }
  
  if((count < 4) || ((8 <= count)&&(count < 12)))
  {
    rect2->color = 0x01;
  } 
  else
  {
    rect2->color = 0x04;
  }
  
  if((count==1) || (count==4) || (count==5) || (count==8) || (count==9) || (count==12) || (count==13))  //combine of 1, 4, 8
  {
    rect3->color = 0x01;
  } 
  else
  {
    rect3->color = 0x04;
  }
  
  if(count%2==0)  //even off
  {
    rect4->color = 0x01;
  } 
  else
  {
    rect4->color = 0x04;
  }
  
  lock_fb_info(info);
  sys_fillrect(info, rect1);
  sys_fillrect(info, rect2);
  sys_fillrect(info, rect3);
  sys_fillrect(info, rect4);
  unlock_fb_info(info);
  
  if(state == 0)
  {
    timer_setup(&timer, timer_function,0);
    mod_timer(&timer, jiffies + HZ/frequency);
  }
  
}

static int __init mycounter_init(void) {
  int result;

  /* Registering device */
  result = register_chrdev(mycounter_major, "mycounter", &mycounter_fops);
  if (result < 0) {
    printk(KERN_ALERT "mycounter: cannot obtain major number %d\n", mycounter_major);
    return result;
  }
  //tmd = kmalloc(sizeof(struct timer_data)*10, GFP_KERNEL); // create 5 tmds
  //timer = kmalloc(sizeof(struct timer_data), GFP_KERNEL);
  /* Allocating ktimer for the buffer */
  mycounter_buffer = kmalloc(capacity*10, GFP_KERNEL);
  if (!mycounter_buffer) {
    printk(KERN_ALERT "Insufficient kernel memory\n");
    result = -ENOMEM;
    goto fail;
  }
  memset(mycounter_buffer, 0, capacity);
  mycounter_len = 0;

  printk(KERN_ALERT "Inserting mycounter module\n");
  
  //seconds = 1/(float)frequency;
  timer_setup(&timer, timer_function,0);
  mod_timer(&timer, jiffies + HZ/frequency);
  //strcpy(tmd[i].timer_msg,message);
  
    printk(KERN_INFO "Hello framebuffer!\n");
    /*printk(KERN_INFO "registered framebuffers: %d\n", num_registered_fb);*/

    rect1 = kmalloc(sizeof(struct fb_fillrect), GFP_KERNEL);
    rect1->dx = 100;
    rect1->dy = 100;
    rect1->width = 150;
    rect1->height = 150;
    rect1->color = 0x01;  //blue
    rect1->rop = ROP_COPY;
    
    rect2 = kmalloc(sizeof(struct fb_fillrect), GFP_KERNEL);
    rect2->dx = 275;
    rect2->dy = 100;
    rect2->width = 150;
    rect2->height = 150;
    rect2->color = 0x01;
    rect2->rop = ROP_COPY;
    
    rect3 = kmalloc(sizeof(struct fb_fillrect), GFP_KERNEL);
    rect3->dx = 450;
    rect3->dy = 100;
    rect3->width = 150;
    rect3->height = 150;
    rect3->color = 0x01;
    rect3->rop = ROP_COPY;
    
    rect4 = kmalloc(sizeof(struct fb_fillrect), GFP_KERNEL);
    rect4->dx = 625;
    rect4->dy = 100;
    rect4->width = 150;
    rect4->height = 150;
    rect4->color = 0x04;  //red
    rect4->rop = ROP_COPY;

    if (num_registered_fb > 0) {
        info = get_fb_info(0);
        /* Must acquire the framebuffer lock before drawing */
        lock_fb_info(info);
        /* Reset the screen */
        fb_blank(info, FB_BLANK_NORMAL);
        /* Draw the rectangle! */
        sys_fillrect(info, rect1);
        sys_fillrect(info, rect2);
        sys_fillrect(info, rect3);
        sys_fillrect(info, rect4);
        /* Unlock mutex */
        unlock_fb_info(info);
    } else {
        printk(KERN_ERR "No framebuffers detected!\n");
    }
    
    /* Cleanup */
    //kfree(rect1);
    //kfree(rect2);
    //kfree(rect3);
    //kfree(rect4);
    printk(KERN_INFO "Module init complete\n");

  return 0;

fail:
  mycounter_exit();
  return result;
}

static void __exit mycounter_exit(void) {

  del_timer(&timer);

  /* Freeing the major number */
  unregister_chrdev(mycounter_major, "mycounter");

  /* Freeing buffer memory */
  if (mycounter_buffer) {
    kfree(mycounter_buffer);
  }
/*
  if (tmd) {
    kfree(tmd);
  }
*/
  printk(KERN_ALERT "Removing mycounter module\n");
}

static int mycounter_open(struct inode *inode, struct file *filp) {
  // printk(KERN_INFO "open called: process id %d, command %s\n", current->pid,current->comm);
  /* Success */
  return 0;
}

static int mycounter_release(struct inode *inode, struct file *filp) {
  // printk(KERN_INFO "release called: process id %d, command %s\n", current->pid,current->comm);
  /* Success */
  return 0;
}

static ssize_t mycounter_read(struct file *filp, char *buf, size_t count1,
                           loff_t *f_pos) {
  //unsigned long int time_left = 0;
  //int i;
  char *tbptr = kmalloc(capacity, GFP_KERNEL);
  // char fbuf[256], *fbptr =fbuf;
  //char *fbptr = kmalloc(capacity*10, GFP_KERNEL);

  // Prevents reading more than needed
  ssize_t bytes = count1 < (capacity -(*f_pos)) ? count1 : (capacity - (*f_pos));
  //memset(fbptr, 0, capacity*10);
  
  /*
  for (i = 0; i < 10; i++) {
    if (tmd[i].active) {
      time_left = (tmd[i].timer.expires - jiffies)/HZ;
      sprintf(tbptr, "%s %lu\n", tmd[i].timer_msg, time_left);
      strcat(fbptr, tbptr);
    }
  }
  strcpy(mycounter_buffer, fbptr);
  */
  
  if(mode==0 && state==0)
    sprintf(tbptr,  "current value:%d\ncurrent frequency:%lu\ncurrent direction:up\ncounter state:running\n",count,frequency);
  else if(mode==0 && state==1)
    sprintf(tbptr, "current value:%d\ncurrent frequency:%lu\ncurrent direction:up\ncounter state:stopped\n",count,frequency);
  else if(mode==1 && state==0)
    sprintf(tbptr, "current value:%d\ncurrent frequency:%lu\ncurrent direction:down\ncounter state:running\n",count,frequency);
  else if(mode==1 && state==1)
    sprintf(tbptr, "current value:%d\ncurrent frequency:%lu\ncurrent direction:down\ncounter state:stopped\n",count,frequency);  
  
  strcpy(mycounter_buffer, tbptr);
  if (copy_to_user(buf, mycounter_buffer, bytes)) {
    return -EFAULT;
  }
    
  
  //kfree(fbptr);
  kfree(tbptr);
  (*f_pos) += bytes;
  return bytes;
}

static ssize_t mycounter_write(struct file *filp, const char *buf, size_t count,
                            loff_t *f_pos) {
  int temp;
  int ret;
  //int i;
  //int modded = 0, added = 0;
  //int num_active_timers = 0;
  char tbuf[256], *tbptr = tbuf;
  char *cp_buf = kmalloc(sizeof(char), GFP_KERNEL);
  //char *message;
  //char *sec;
  char *flag_start;
  char *sec;
  char *nothing;
  unsigned long freq;
  
  /* end of buffer reached */
  if (*f_pos >= capacity) {

    return -ENOSPC;
  }


  /* do not go over the end */
  if (count > capacity - *f_pos) count = capacity - *f_pos;

  if (copy_from_user(mycounter_buffer + *f_pos, buf, count)) {
    return -EFAULT;
  }

  for (temp = *f_pos; temp < count + *f_pos; temp++)
    tbptr += sprintf(tbptr, "%c", mycounter_buffer[temp]);
  
  *f_pos += count;
  mycounter_len = *f_pos;

  strcpy(cp_buf, tbuf);
  flag_start = strsep(&cp_buf, ";");
  strcpy(flag,flag_start);                           //attention that flag includes \n. Check this by printk flag and see a blank row betw
  
  if(strcmp(flag,"up\n")==0)             
  {
    mode = 0;
  }
  else if(strcmp(flag,"down\n")==0)
  {
    mode = 1;
  }
  else if(strcmp(flag,"stop\n")==0)
  {
    //time_left = (timer.expires - jiffies)/HZ;
    //printk(KERN_ALERT "%lu\n",time_left);
    //del_timer(&timer);
    mod_timer(&timer, jiffies + 1000000*HZ);    //large number seems like stop
    state = 1;
  }
  else if(strcmp(flag,"go\n")==0)
  {
    //timer_setup(&timer, timer_function,0);
    //mod_timer(&timer, jiffies + time_left*HZ);
    mod_timer(&timer, jiffies + HZ/frequency);
    state = 0;
  }
  else if(flag[0]=='f')
  { 
    freq = frequency;
    nothing = strsep(&flag_start,"f");
    sec = strsep(&flag_start,"/n");
    ret = kstrtoul(sec,0,&frequency);
    //printk(KERN_ALERT "%lu\n",frequency);
    if(frequency>=1 && frequency <=9)
    {
      //mod_timer(&timer, jiffies + msecs_to_jiffies(1000/frequency));
      mod_timer(&timer, jiffies + HZ/frequency);
    }
    else
    {
      frequency = freq;
      printk(KERN_ALERT "please echo right command :f1 to f9");
    }
  }
  else
  {
    printk(KERN_ALERT "please echo right command like up, down, stop, go, f1 to f9");
  }
  
  kfree(cp_buf);
  return count;
}
