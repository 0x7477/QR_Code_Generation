#include <stdio.h>

int gf2dec[255];
int dec2gf[255];

void generateTable()
{
    int val = 1;
    for(int i = 0; i < 255; i++)
    {
        gf2dec[i] = val;
        dec2gf[val] = i;
        val *= 2;
        if(val > 255) val = val ^ 285;
    }
}



class poly
{
    public:

        int constant;
        int degree;

        poly(int c, int d)
        {
            constant = c;
            degree = d;
        }

        void print()
        {
            if(degree == 0) printf("+ %d ",constant);
            else printf("+ %dx^%d ",constant,degree);
        }

        void print_alpha()
        {
            if(degree == 0) printf("+ %d ",constant);
            else printf("+ a^%dx^%d ",dec2gf[constant],degree);
        }

        void multiply_x(int c) {degree += c;}
};

std::vector<poly> multiply_poly(std::vector<poly> a, poly b)
{
    std::vector<poly> sum = std::vector<poly>();
    int b_alpha = dec2gf[b.constant];

    for(int i = 0; i < a.size(); i++)
    {
        int a_alpha = dec2gf[a[i].constant];

        sum.push_back(poly(gf2dec[(a_alpha + b_alpha) % 255],a[i].degree));
    }

    return sum;
}

std::vector<poly> xor_polys(std::vector<poly> a, std::vector<poly> b)
{
    std::vector<poly> sum;

    for(int i = 0; i < std::max(a.size(), b.size()); i++)
    {
        if(i >= b.size())
        {
            sum.push_back(a[i]);
            continue;
        }
        if(i >= a.size())
        {
            sum.push_back(b[i]);
            continue;
        }

        if(i != 0)sum.push_back(poly(a[i].constant ^ b[i].constant,a[i].degree));
    }

    return sum;
}