void move_car(uint32_t** ptr_car, char direct, int scr_xres)
{
  switch(direct)
  {
    case UP:
      {
        *ptr_car -= scr_xres;
        break;
      }
      case DOWN:
      {
        *ptr_car += scr_xres;
        break;
      }
      case LEFT:
      {
        *ptr_car -= 1;
        break;
      }
      case RIGHT:
      {
        *ptr_car += 1;
        break;
      }
  }
}

char set_opposite_direct(char direct, char direct_prev, char* ptr_opposite_direct)
{
    switch(direct)
    {
      case UP:
      {
        *ptr_opposite_direct = DOWN;
        if( direct_prev == LEFT || direct_prev == RIGHT )
            return 1;
        else
            return 0;
      }
      case DOWN:
      {
        *ptr_opposite_direct = UP;  
        if( direct_prev == LEFT || direct_prev == RIGHT )
            return 1;
        else 
            return 0;
        break;
      }
      case LEFT:
      {
        *ptr_opposite_direct = RIGHT;  
        if( direct_prev == UP || direct_prev == DOWN )
            return 1;
        else 
            return 0;
      }
      case RIGHT:
      {
        *ptr_opposite_direct = LEFT;  
        if( direct_prev == UP || direct_prev == DOWN )
            return 1;
        else 
            return 0;
      }
    }
}

char is_cross(uint32_t * ptr_car_p1, uint32_t* ptr_car_p2, char direct_p1, char direct_p2, int scr_xres)
{
    uint32_t* car_p1[40];
    int index = 0;
    switch (direct_p1)
    {
        case UP:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                   car_p1[index] = ptr_car_p1 + j + i*scr_xres;
                   index ++;
                }
            }
            break;
        case DOWN:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                   car_p1[index] = ptr_car_p1 + j + i*scr_xres;
                   index ++;
                }
            }
            break;
        case LEFT:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                   car_p1[index] = ptr_car_p1 + i + j*scr_xres;
                   index ++;
                }
            }
            break;
        case RIGHT:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                   car_p1[index] = ptr_car_p1 + i + j*scr_xres;
                   index ++;
                }
            }
            break;
    }  
    index = 0;
    switch (direct_p2)
    {
        case UP:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = ptr_car_p2 + j + i*scr_xres;
                   if(car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                 }
            }
            break;
        case DOWN:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = ptr_car_p2 + j + i*scr_xres;
                   if(car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                }
            }
            break;
        case LEFT:
            for(int i = 0; i>-8; i--)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = ptr_car_p2 + i + j*scr_xres;
                   if(car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                }
            }
            break;
        case RIGHT:
            for(int i = 0; i<8; i++)
            {
                for(int j = -2; j<=2; j++)
                {
                  for(int indx = 0; indx < 40; indx++)
                  {
                   uint32_t* result = ptr_car_p2 + i + j*scr_xres;
                   if(car_p1[indx] == result)
                   {
                     return 1;
                   }
                  }
                }
            }
            break;
    }
    return 0;
}
