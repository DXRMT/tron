void invert_four_bytes(char *ptr)
{
  char tmp=ptr[0];
  ptr[0]=ptr[3];
  ptr[3]=tmp;
  
  tmp=ptr[1];
  ptr[1]=ptr[2];
  ptr[2]=tmp;
}

void set_keypress(void)
{
  struct termios new_settings;

	tcgetattr(0,&stored_settings);

	new_settings = stored_settings;

	new_settings.c_lflag &= (~ICANON & ~ECHO);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;

	tcsetattr(0,TCSANOW,&new_settings);
	return;
}

void reset_keypress(void)
{
  tcsetattr(0,TCSANOW,&stored_settings);
	return;
}
int handler(int none)
{
  fprintf(stdout,"\t\t\t\t\t\t\tloss\n");
  exit(0);
}