#include <stdio.h>

main(int argc,char **argv)
{
FILE *fin,*fout;
int x,y;
int width,height;
int r,v,g;

fin=fopen(argv[1],"r");
fout=fopen(argv[2],"w");

fscanf(fin,"P3\n");
fscanf(fin,"%d %d\n",&width,&height);
fscanf(fin,"255\n");

fprintf(fout,"png_color cmap[%d][%d]={\n",width,height);
  for(y=0;y<height;y++) {
  fprintf(fout,"{");
	for(x=0;x<width;x++) {
	fscanf(fin,"%d\n%d\n%d\n",&r,&v,&g);
  	fprintf(fout,"{ %d,%d,%d},",r,v,g);
	if(x%8==0) fprintf(fout,"\n");
  }	
  fprintf(fout,"},\n");
} 
  fprintf(fout,"};\n");
fclose(fin);
fclose(fout);

}

