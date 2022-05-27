#include "func.h"

int main(int argc, char* argv[])
{ 
  int mode_sync = 0; // 0 - not syncing, 1 - syncing
  signal(SIGINT, handler);
  if(argc < 4)
  {
      printf("Use: ./Tron.exe <xres> <yres> <opponent's ip> <0-nsync, 1-sync>\n");
      return -1;
  }
  else if(argc == 5)
      mode_sync = atoi(argv[4]);

  //init screen
  printf("\033c"); //clear stdout
  set_keypress(); // set noecho and cbreak modes stdin

  int fb;
  struct fb_var_screeninfo info;
  size_t fb_size, map_size, page_size;
  uint32_t *ptr;
  
  page_size = sysconf(_SC_PAGESIZE);
  
  if ( 0 > (fb = open("/dev/fb0", O_RDWR))) 
  {
    printf("open");
    reset_keypress();
    return __LINE__;
  }

  if ( (-1) == ioctl(fb, FBIOGET_VSCREENINFO, &info)) 
  {
    printf("ioctl");
    close(fb);
    reset_keypress();
    return __LINE__;
  }

  fb_size = sizeof(uint32_t) * info.xres_virtual * info.yres_virtual;
  map_size = (fb_size + (page_size - 1 )) & (~(page_size-1));

  ptr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
  if ( MAP_FAILED == ptr ) 
  {
    printf("mmap");
    close(fb);
    reset_keypress();
    return __LINE__;
  }
  
  int xres_area = atoi(argv[1]);
  int yres_area = atoi(argv[2]);
#ifdef WITHOUTCURSOR
  if(xres_area + 2 > info.xres || yres_area + 2 > info.yres 
          || xres_area <= 10 || yres_area <= 10) //considering the boundaries 
#else 
  if(xres_area + 2 > info.xres || yres_area + 2 > info.yres-32
          || xres_area <= 10 || yres_area <= 10) //considering the boundaries and first line
#endif
  {
    munmap(ptr, map_size);
    close(fb);
    reset_keypress();
    printf("Not supported size of area");
    return __LINE__;
  }

  //init socket
  int sockfd;
  struct sockaddr_in opponent_addr, player_addr;
  
  if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    munmap(ptr, map_size);
    close(fb);
    reset_keypress();
    printf("Socket creation failed");
    return __LINE__;
  }
  
  opponent_addr.sin_family = AF_INET;
  opponent_addr.sin_port = htons(12345);
  opponent_addr.sin_addr.s_addr = inet_addr(argv[3]);
 
  player_addr.sin_family = AF_INET;
  player_addr.sin_port = htons(12345);
  player_addr.sin_addr.s_addr = get_local_ip(opponent_addr.sin_addr.s_addr);

  if(player_addr.sin_addr.s_addr == 0 || 
    player_addr.sin_addr.s_addr == opponent_addr.sin_addr.s_addr)
  {
    close(sockfd);
    munmap(ptr, map_size);
    close(fb);
    reset_keypress();
    printf("Incorrect opponent's ip\n");
    printf("Your ip:%s", inet_ntoa(player_addr.sin_addr));
    return __LINE__;
  }

  if(bind(sockfd, (struct sockaddr*)&player_addr, sizeof(player_addr)) < 0)
  {
    close(sockfd);
    munmap(ptr, map_size);
    close(fb);
    reset_keypress();
    printf("Bind error");
    return __LINE__;
  }

  //init threads
  pthread_t tid_control, tid_syncing;
  pthread_attr_t attr;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  
  if( pthread_attr_init(&attr) != 0 )
  {
    close(sockfd);
    munmap(ptr, map_size);
    close(fb);
    reset_keypress();
    fprintf(stderr, "Error of init attr\n");
    return 1;
  }

  // init players
  uint32_t* ptr_car_p1 = ptr + 1 + info.xres_virtual* 3;
  uint32_t* ptr_car_p2 = ptr + xres_area + 
      info.xres_virtual*(yres_area-2);
  char direct_p1 = RIGHT;
  char direct_p2= LEFT;
  char direct_prev_p1 = RIGHT;
  char direct_prev_p2 = LEFT;
  char opposite_direct_p1;
  char opposite_direct_p2;
  char is_need_additional_pixel_p1 = 0;
  char is_need_additional_pixel_p2 = 0;
  char who_lose[] = {0,0};  //who_lose[0] - first player 
  char index_player = 0;    //who_lose[1] - second player
  char is_ready_p1 = 0;     //if index_player == 0 then player is master else slave
  char is_ready_p2 = 0;
  char tmp;

  struct args_keys args1 = {sockfd, &direct_p1, &is_ready_p1, &opponent_addr, &mutex};
  struct args_keys args2 = {sockfd, &direct_p2, &is_ready_p2, &opponent_addr, &mutex};  
  void (*control_thread) (struct args_keys* args);
  void (*syncing_thread) (struct args_keys* args);
  if (mode_sync)
  {
      control_thread = control_thread_sync;
      syncing_thread = interaction_thread_sync;
  }
  else
  {
      control_thread = control_thread_nsync;
      syncing_thread = interaction_thread_nsync;

  }

  //invert bytes for compliance ips
  char opponent_ip[sizeof(unsigned long)];
  char player_ip[sizeof(unsigned long)];
  
  *((unsigned long*)opponent_ip) = opponent_addr.sin_addr.s_addr;
  *((unsigned long*)player_ip) = player_addr.sin_addr.s_addr;
  
  invert_four_bytes(opponent_ip);
  invert_four_bytes(player_ip);
  
  if(*((int*)player_ip) < *((int*)opponent_ip))
  {
      args1.ptr_direct = &direct_p2; args1.ptr_is_ready_player = &is_ready_p2;
      args2.ptr_direct = &direct_p1; args2.ptr_is_ready_player = &is_ready_p1;
      index_player = 1;
  }
  
  if( pthread_create(&tid_control, &attr,(void *)control_thread, &args1) != 0 )
  {
    close(sockfd);
    munmap(ptr, map_size);
    close(fb);
    reset_keypress();
    fprintf(stderr, "Error of create thread\n");
    return 2;
  }

  if( pthread_create(&tid_syncing, &attr,(void *)syncing_thread, &args2) != 0 )
  {
    close(sockfd);
    munmap(ptr, map_size);
    close(fb);
    reset_keypress();
    fprintf(stderr, "Error of create thread\n");
    return 2;
  }

  // wait
  struct timeb tb; 
  time_t start_t; 

  ftime(&tb);
  start_t = tb.time;
  char game_start = 1;
  while(is_ready_p1 != 1 || is_ready_p2 != 1)
  {
      if(tb.time - start_t >= 10)
      {
        work_flag = 0;
        game_start = 0;
        is_ready_p1 = 1;
        is_ready_p2 = 1;
      }
      else
      {
        usleep(1);
        ftime(&tb);
      }
  }
  start_flag = 1;

  // start game 
  ftime(&tb);
  unsigned start_m = tb.millitm;
  uint32_t background_color = ptr_car_p2[0];
  draw_car(ptr_car_p1, direct_p1, RED, info.xres_virtual);
  draw_car(ptr_car_p2, direct_p2, BLUE, info.xres_virtual);
  draw_area(ptr , xres_area, 
          yres_area, info.xres_virtual);
 
  while(work_flag)
  {
    if(mode_sync) //with sync
    {
        if(index_player == 0) //player is master
        {
            pthread_mutex_lock(&mutex);
            is_need_additional_pixel_p1 = set_opposite_direct(direct_p1, direct_prev_p1, &opposite_direct_p1);
            need_answer = 1;
            if(direct_prev_p1 != opposite_direct_p1)
                tmp = direct_p1 + number_step % 2;
            else //player is slave
                tmp = direct_prev_p1 + number_step % 2;
            while(need_answer)
            {
                sendto(sockfd, &tmp, 1, 0,(struct sockaddr*)(&opponent_addr), sizeof(opponent_addr));
                usleep(50);
            }
            is_need_additional_pixel_p2 = set_opposite_direct(direct_p2, direct_prev_p2, &opposite_direct_p2);
        }
        else
        {
            need_answer = 1;
            while(need_answer)
                usleep(5);
            pthread_mutex_lock(&mutex);
            is_need_additional_pixel_p2 = set_opposite_direct(direct_p2, direct_prev_p2, &opposite_direct_p2);
            if(direct_prev_p1 != opposite_direct_p1)
                tmp = direct_p2 + number_step % 2;
            else
                tmp = direct_prev_p2 + number_step % 2;
            for(int i = 0; i<10; i++)
                sendto(sockfd, &tmp, 1, 0, (struct sockaddr*)(&opponent_addr), sizeof(opponent_addr));
            is_need_additional_pixel_p1 = set_opposite_direct(direct_p1, direct_prev_p1, &opposite_direct_p1);
        }
    }
    else //without sync
    {
        pthread_mutex_lock(&mutex);
        is_need_additional_pixel_p1 = set_opposite_direct(direct_p1, direct_prev_p1, &opposite_direct_p1);
        is_need_additional_pixel_p2 = set_opposite_direct(direct_p2, direct_prev_p2, &opposite_direct_p2);
    }
    // move first player's car 
    if(direct_prev_p1 != opposite_direct_p1)
    {
      delete_car(ptr_car_p1, direct_prev_p1, info.xres_virtual, background_color);
      *ptr_car_p1 = RED;
      if(is_need_additional_pixel_p1)
      {
        move_car(&ptr_car_p1, direct_p1, info.xres_virtual);
        *ptr_car_p1 = RED;
        is_need_additional_pixel_p1 = 0;
      }
      move_car(&ptr_car_p1, direct_p1, info.xres_virtual);
      if(draw_car(ptr_car_p1, direct_p1, RED, info.xres_virtual))
      {
        work_flag= 0;
        who_lose[0] = 1;
      }
      direct_prev_p1 = direct_p1;  
    }
    else
    {
      delete_car(ptr_car_p1, direct_prev_p1, info.xres_virtual, background_color);
      *ptr_car_p1 = RED;
      move_car(&ptr_car_p1, direct_prev_p1, info.xres_virtual);
      if(draw_car(ptr_car_p1, direct_prev_p1, RED, info.xres_virtual))
      {
        work_flag = 0;
        who_lose[0] = 1;
      }
    }
    // move second player's car
    if(direct_prev_p2 != opposite_direct_p2)
    {
      delete_car(ptr_car_p2, direct_prev_p2, info.xres_virtual, background_color);
      *ptr_car_p2 = BLUE;
      if(is_need_additional_pixel_p2)
      {
        move_car(&ptr_car_p2, direct_p2, info.xres_virtual);
        *ptr_car_p2 = BLUE;
        is_need_additional_pixel_p2 = 0;
      }
      move_car(&ptr_car_p2, direct_p2, info.xres_virtual);
      if(draw_car(ptr_car_p2, direct_p2, BLUE, info.xres_virtual))
      {
        work_flag= 0;
        who_lose[1] = 1;
      }
      direct_prev_p2 = direct_p2;
    }
    else
    {
      delete_car(ptr_car_p2, direct_prev_p2, info.xres_virtual, background_color);
      *ptr_car_p2 = BLUE;
      move_car(&ptr_car_p2, direct_prev_p2, info.xres_virtual);
      if(draw_car(ptr_car_p2, direct_prev_p2, BLUE, info.xres_virtual))
      {
        work_flag = 0;
        who_lose[1] = 1;
      }
    }
    pthread_mutex_unlock(&mutex);
    if(mode_sync && index_player == 0 || mode_sync == 0)
    {
      ftime(&tb);
      usleep(62500 - (((unsigned)(tb.millitm  - start_m) < 10 ) ? (tb.millitm - start_m)*1000 : 7500)); 
      ftime(&tb);
      start_t = tb.millitm;
    }
    else
        usleep(20000);
    number_step++;
  }
  
  if(is_cross(ptr_car_p1,ptr_car_p2, direct_prev_p1, direct_prev_p2, info.xres_virtual))
  {
    work_flag = 0;
    who_lose[0] = 1;
    who_lose[1] = 1;
  }

  pthread_kill(tid_control, 17); 
  pthread_kill(tid_syncing, 17);

  if(who_lose[index_player] == 0 && who_lose[0] != who_lose[1])
    fprintf(stdout,"\t\t\tvictory!!!\n");
  else if(game_start == 1)
    fprintf(stdout,"\t\t\t\t\t\t\tloss\n");

  close(sockfd);
  munmap(ptr, map_size);
  close(fb);
  reset_keypress();
  return 0;
}