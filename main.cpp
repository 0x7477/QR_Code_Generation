#include <stdio.h>
#include <iostream>
#include <vector>

#include "reed_solomon.cpp"

#define INPUT "http://192.168.255.103/abcde.apk"


std::string char2bin(char c)
{
    std::string data = "";

    for(int i = 0; i < 8; i++)
    {
        data += ((c & 0b10000000) == 0)?"0":"1";
        c = c << 1;
    }
    
    return data;
}


std::string string2bin(std::string input)
{
    std::string data = "";
    for(int i = 0; i < input.size(); i++)
    {
        data += char2bin(input[i]);
    }
    return data;
}

void print_white()
{
    printf("\033[37;47m  ");
    printf("\033[0m");
}
void print_black()
{
    printf("\033[37;30m  ");
    printf("\033[0m");
}
void print_blue()
{
    printf("\033[37;46m  ");
    printf("\033[0m");
}

int main(int agrc, char** argv)
{
    generateTable();

    std::string input = INPUT;

    //Version 2 encoding
    //Mode = Byte

    int image_width = 25;
    
    int capacity = 34 * 8; //272 bits to be filled
    int ec_codewords_per_block = 10;

    std::string mode = "0100"; // Byte Mode
    std::string count = char2bin((char)input.size());
    std::string data = string2bin(input);

    std::string terminator = "0000";

    int missingbytes = capacity - mode.size() - count.size() - data.size() - terminator.size();


    std::string byte_padding = "";
    for(int i = 0; i < missingbytes / 8; i++)
    {
        byte_padding += char2bin((char)(i%2)?(0xec):(0x11));
    }
    
    std::string complete = mode + count + data + terminator + byte_padding;

    std::vector<poly> polynomial = std::vector<poly>();
    std::vector<poly> generator = 
    {
        poly(gf2dec[0],10),
        poly(gf2dec[251],9),
        poly(gf2dec[67],8),
        poly(gf2dec[46],7),
        poly(gf2dec[61],6),
        poly(gf2dec[118],5),
        poly(gf2dec[70],4),
        poly(gf2dec[64],3),
        poly(gf2dec[94],2),
        poly(gf2dec[32],1),
        poly(gf2dec[45],0),
    };

    int byte = 0;
    for(int i = 0; i < complete.size(); i+=8)
    {
        unsigned char c = 0;
        for(int j = 0; j < 8; j++)
        {
            c +=(complete[i+j] == '1')?1:0;
            if(j != 7)c = c << 1;
        }
        polynomial.push_back(poly(c,(complete.size()-i)/8 -1));
        byte++;
    }


    //multiplying by x^n where n is equal to generator length

    for(poly& p : polynomial) p.multiply_x(generator.size()-1);
    for(poly& p : generator)  p.multiply_x(polynomial.size()-1);

    std::vector<poly> step = polynomial;
    std::vector<poly> lastb = polynomial;

    for(int i = 0; i < polynomial.size(); i++)
    {
        step = multiply_poly(generator,step[0]);
        step = xor_polys(lastb,step);
        lastb = step;
    }

    for(poly& p : step) complete += char2bin((char)p.constant);
    
    char table[image_width][image_width];

    for(int x = 0; x < image_width; x++)
    {
        for(int y = 0; y < image_width; y++)
        {
            table[x][y] = 2;
        }
    }

    for(int i = 0; i < image_width; i++)
    {
        table[6][i] = i % 2 == 1;
        table[i][6] = i % 2 == 1;
    }

    int offset[3][2] = {{-1,-1},{image_width - 8,-1},{-1,image_width -8}};
    for(int a = 0; a < 3; a++)
    {
        for(int i = 0; i < 4; i++)
        {
            for(int j = i; j < 9 -i; j++)
            {
                for(int l = i; l < 9-i; l++)
                {
                    int x = j + offset[a][0];
                    int y = l + offset[a][1];
                    if(x < 0 || y < 0 || x >= image_width || y >= image_width) continue;
                    table[x][y] = i%2==0;
                }
            }
        }
    }

    for(int i = 0; i < 3; i++)
    {
        for(int x = i; x < 5-i; x++)
        {
            for(int y = i; y < 5-i; y++)
            {
                table[image_width - 9 + x][image_width - 9 + y] = i%2==1;
            }
        }
    }

    for(int i = 0; i < 9; i++)
    {
        table[image_width - i][8] = i != 8;
        if(i != 8)table[8][image_width -i-1] = 1;

        if(i == 6) continue;
        table[8][i] = true;
        table[i][8] = true;
    }

    int x = image_width -1;
    int y = image_width -1;

    int mode1[2][2] = {{-1,0},{1,-1}};
    int mode2[2][2] = {{-1,0},{1,1}};

    int up = 1;
    int right = 0;
    int character = 0;
    for(int i = 0; i < complete.size(); i++)
    {
        table[x][y] = (complete[i] != '1') + 10;
        character++;
        while(table[x][y] != 2)
        {
            if(up == 1)
            {
                x += mode1[right][1];
                y += mode1[right][0];
                right = (right +1) %2;

                if(x < 0)
                {
                    up = 0;
                    y-=2;
                    x++;
                    
                    bool freespace = false;
                    for(int n = 0; n < image_width; n++)
                    {
                        if(table[n][y] == 2)
                        {
                            freespace = true;
                            break;
                        }
                    }
                    if(!freespace) y--;
                } 
            }
            else
            {
                x += mode2[right][1];
                y += mode2[right][0];
                right = (right +1) %2;

                if(x >= image_width)
                {
                    up = 1;
                    x--;
                    y-=2;
                }
            }
        }
    }

    


    for(int x_ = 0; x_ < image_width; x_++)
    {
        for(int y_ = 0; y_ < image_width; y_++)
        {
            if(table[x_][y_] >= 10 && (x_+y_) % 2 == 0)
            {
                table[x_][y_] = !(table[x_][y_] % 2);
            }
            if(table[x_][y_] == 2)
            {
                table[x_][y_] = (x_+y_) % 2 == 1;
            }
        }
    }

    std::string format_string = "000100000111011";

    for(int i = 0; i < 7; i++) table[image_width - i-1][8] = format_string[i] == '1';
    for(int i = 7; i < 15; i++) table[8][image_width - 15 + i] = format_string[i] == '1';
    for(int i = 0; i < 6; i++) table[8][i] = format_string[i] == '1';
    for(int i = 6; i < 8; i++) table[8][i+1] = format_string[i] == '1';
    table[7][8] = format_string[8] == '1';
    for(int i = 9; i < 15; i++) table[14-i][8] = format_string[i] == '1';    
    
    for(int x_ = 0; x_ < image_width; x_++)
    {
        for(int y_ = 0; y_ < image_width; y_++)
        {
            (table[x_][y_] % 2 ==1)?print_white():print_black();
        }
        printf("\n");
    }
    return 0;
}