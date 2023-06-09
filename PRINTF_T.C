void my_printf(char *format, ...);
void print_char(char c);
void print_decimal(int decimal);
void print_int_min();

#define BASE (0xb8000000 + 160 * 12) /* 第12行0列的地址 */
#define INT_MIN -32768               /* 16位整型数的最小值，输出时需要特殊处理 */

unsigned int str_index = 0;          /* 屏幕上单个字符相对于BASE的偏移位置 */

int main()
{
  /*
   * 由于输出位置固定，因此连续调用两次my_printf函数后，第二次的输出会覆盖第一次
   * 所以每次运行只调用一次my_printf函数，而注释其他的调用语句
   */
  my_printf("In DECIMAL, %d - %d = %d.\n", -32767, 1, -32768);
  my_printf("In HEX, %dH + %dH = %c%cH.", 90, 11, 'A', '1');

  return 0;
}

void my_printf(char *format, ...)
{
  char *p = format;
  unsigned int param_index = 0; /* format之后的参数在栈中的下标，从0开始 */

  while (*p)              /* 遍历format，遇到'\0'就退出 */
  {
    if (*p == '%')        /* 遇到'%'字符，则读取下一个字符 */
    {
      p++;
      if (*p == 'c')
      {
        /*
         * 将栈中当前参数（字型，2字节）按字符类型输出
         * 对于这个2字节的参数，我们只需要代表字符的那个字节
         * 8086是小端机器，因此实际需要输出的字符保存在低位字节
         */
        char ch = *(char *)(_BP + 6 + 2 * param_index);   /* 获取低位字节，详见后文分析 */
        param_index++;
        print_char(ch);
      }
      else if (*p == 'd')
      {
        /*
         * 将栈中当前参数（字型，2字节）按十进制整型输出
         * 这2个字节都有意义，它们共同构成一个16位整型数据
         */
        int decimal = *(int *)(_BP + 6 + 2 * param_index); /* 见后文分析 */
        param_index++;
        print_decimal(decimal);
      }
      p++;
    }
    /*
     * 遇到换行符'\n'，则令str_index变为代表下一行起始位置的值
     * 注意不要将'\n'拆分为'\\'和'n'两个字符
     * 因为'\n'在格式控制字符串format中，本就已经被转义为一个单独的换行符
     */
    else if (*p == '\n')
    {
      str_index += (80 - str_index);
      p++;
    }
    else
    {
      print_char(*p);
      p++;
    }
  }
}

/* 在相对于BASE偏移量为str_index的位置，打印单个字符c */
void print_char(char c)
{
  /*
   * 重坑提醒：一定不要遗漏far！
   * 在16位的8086处理器中，int类型长度为一个字（16位）
   * 如果不写far，那么地址会被当做字型（16位）而不是双字型（32位）来处理
   * 而BASE = 0xb8000000 +  160 * 12，必须用双字型才能完整表示
   * 如果使用默认的字类型，则高位字将会丢失，实际地址将会是(160 * 12 + 2 * index)，
   * 而不是(0xb8000000 + 160 * 12 + 2 * index)
   */
  *(char far *)(BASE + 2 * str_index) = c;
  str_index++;
}

/* 在相对于BASE偏移量为index的位置，输出一个整数 */
void print_decimal(int decimal)
{
  if (decimal == 0)
  {
    print_char('0');
  }
  else
  {
    /* INT_MIN（-32768 ，16位整型数最小值）没有对应的相反数（16位整型最大值为32767），需单独处理 */
    if (decimal == INT_MIN)
    {
      print_int_min();
    }
    else if (decimal < 0)
    {
      print_char('-');                /* 对于负数，先输出一个负号'-' */
      decimal = -decimal;             /* 将decimal变为相反数（正数），之后进行整除和取余 */
    }
    if (decimal > 0)
    {
      /* 排除INT_MIN的情形 */
      /* 逐个获取数位，先入栈后出栈 */
      int count = 0;
      char ch;
      while (decimal)
      {
        int num = decimal % 10;
        decimal /= 10;

        /* 手动实现汇编push指令 */
        _SP -= 2;
        *(int *)_SP = num;

        count++;
      }
      while (count)
      {
        /* 手动实现汇编pop指令 */
        ch = *(char *)_SP; /* 获取代表字符的低位字节 */
        _SP += 2;

        ch += 0x30; /* 将数字变为对应的数字字符 */
        print_char(ch);
        count--;
      }
    }
  }
}
/*
 * 对INT_MIN进行特殊处理
 * 在相对于BASE偏移量为str_index的位置，输出"-32768"
 */
void print_int_min()
{
  *(char far *)(BASE + 2 * str_index) = '-';
  str_index++;
  *(char far *)(BASE + 2 * str_index) = '3';
  str_index++;
  *(char far *)(BASE + 2 * str_index) = '2';
  str_index++;
  *(char far *)(BASE + 2 * str_index) = '7';
  str_index++;
  *(char far *)(BASE + 2 * str_index) = '6';
  str_index++;
  *(char far *)(BASE + 2 * str_index) = '8';
  str_index++;
}
