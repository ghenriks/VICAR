/**
 **	pscript.c:
 **
 **	Purpose:
 **
 **		To convert Vicar images to PostScript text file format,
 **		downloadable to a PostScript-compatible device.
 **
 **	Revision History:
 **
 **		18 MAR 1993: NDR: Converted from FORTRAN to C (Unix Port).
 **		09 JUN 1993: NDR: Fixed bugs in scaling images,
 **                               Incremental subsampling, and Titles.
 **/

#include "vicmain_c"
#include <string.h>	/* for malloc */
#include <stdlib.h>
#include <stdio.h>

void main44(void)
{
	float range[2],offset,slope,availheight,availwidth,aspect,margin;
	int count,def,defpos,defwidth,defheight;
	int scale, numinp;
	int dotitle,numtitles;
	int inunit[3],nl,ns;
	int nlo,nso;
	int nbits,nsb,bitshift;
	int increment;
	int fontpoint;
	int hexcount;
	int i,line,samp,band;
	int pixsize;
	unsigned char *lbuff,*ptr,*endptr;
	float pagesize[2],pagepos[2];
	float width,height,freq;
	char *hexformat, command_line[133], command_part[20];
	char *format, filename[133];
	char outname[133],title[5][132];
	char fontname[133];
	char timestr[80];
	int psize;
	long tim;
	long time();
	char *ctime();
	FILE *outfile;  /* FILE is not very portable... */
	
	zveaction("sa","");

 	/* Find out if the input image isn't BYTE; if so, scale */

	zvunit( inunit, "inp", 1, NULL);
	zvopen( inunit[0], NULL);
	zvget( inunit[0],"pix_size",&psize,NULL);
        zvclose( inunit[0],NULL);

	zvparm( "dnrange", range, &count, &def, 0,0);
	scale = (def == 0) || (psize>1);
	if (scale)
	{
		format = "real";
		offset = range[0];
		slope = 255. / (range[1]+1e-38 - offset);
	}
	else format = "byte";

	zvpcnt( "inp", &numinp );
	for (i=0;i<numinp; i++)
	{
		zvunit( inunit+i, "inp", i+1,NULL);
		zvopen( inunit[i], "u_format", format, NULL);
	}

	zvget( inunit[0], "nl", &nl, "ns", &ns, NULL);
	zvp( "inc", &increment,&count);
	zvpone( "inp", filename, 1, 132 ); 
	zvparm( "title", title, &numtitles, &def, 5,132);
	dotitle = (def == 0) || ! zvptst( "notitle" );

	/** allocate storage buffer **/

	zvpixsizeu(  &pixsize, format, inunit[0] );
	lbuff = (unsigned char *)malloc( (long)pixsize*ns );
	if (!lbuff)
	{
		zvmessage( "unable to allocate line buffer","");
		zabend();
	}
	
	zvpone( "font", fontname, 1, 132 );
	zvp( "point", &fontpoint, &count );
	
	tim = time(0);			/* Get date and time */
	strcpy(timestr, ctime(&tim) );

	nso = ns/increment;
	nlo = nl/increment;
	if (zvptst( "8bits" ) )
	{
		nbits = 8;
		hexcount = 30;
		hexformat="%02x";
	}
	else
	{
		nbits = 4;
		hexcount = 60;
		hexformat="%01x";
	}
	nsb = nso/hexcount;
	bitshift = 8-nbits;
	
	zvparm( "pagesize", pagesize, &count, &def, 0,0);
	
	if (zvptst("wide"))
	{
		float ptmp;
		
		/* landscape option -- switch values */
		ptmp = pagesize[0];
		pagesize[0] = pagesize[1];
		pagesize[1] = ptmp;
	}
	
	zvp( "margin", &margin, &count );
	zvpone( "out", outname, 1, 132 );
	zvparm( "pagepos", pagepos, &count,&defpos, 0,0);
	zvparm( "width", &width, &count, &defwidth, 0,0);
	zvparm( "height", &height, &count, &defheight, 0,0);

	availwidth = pagesize[0] - pagepos[0] - 0.5;
	availheight = pagesize[1] - pagepos[1] - margin -
                       (float) ((numtitles+1) * fontpoint)/72.0 - 0.5;
	aspect = (float) nl / (float) ns;

	if (defwidth == 1)
		if (defheight == 1)
		{					    /* both defaulted */
			width = availwidth;
			height = availheight;
			if (height/width > aspect)	/*force correct aspect*/
				height = width * aspect;
			else
				width = height / aspect;
		}
		else				      /* only width defaulted */
			width = height / aspect;
	else
		if (defheight == 1)		     /* only height defaulted */
			height = width * aspect;
		else					/* neither defaulted  */
			if (height/width > aspect)	/*force correct aspect*/
				height = width * aspect;
			else
				width = height / aspect;
							/*force to fit on page*/
	if (width > availwidth)
	{
		width = availwidth;
		height = width * aspect;
	}
	if (height > availheight)
	{
		height = availheight;
		width = height / aspect;
	}

	if (defpos==1)
	{
		pagepos[0] = (pagesize[0] - width) /2.;
		pagepos[1] = pagesize[1] - (availheight + height + 0.5)/2.0 ;
	}

	outfile = fopen( outname, "w" );
	
	if (!outfile)
	{
		zvmessage( "Error opening output file"," ");
		zabend();
	}

/*
 * Set up the ADOBE POSTSCRIPT (R)  Page description Comments
 */
	fprintf( outfile, "%s\n",	"%!PS-Adobe-2.0");
	fprintf( outfile, "%s\n",	"%%Creator: VICAR Program PSCRIPT");
	fprintf( outfile, "%s%s\n",	"%%Title: ", filename);
	fprintf( outfile, "%s%s",	"%%CreationDate: ", timestr);
	fprintf( outfile, "%s\n",	"%%Pages: 1");
	fprintf( outfile, "%s%s\n",	"%%DocumentFonts: ", fontname);
	fprintf( outfile, "%s\n\n",	"%%EndComments");

/*
 * Set up the PostScript Command definitions
 */

	fprintf( outfile, "%s\n",	" /inch { 72 mul} def");
	fprintf( outfile, "%s%9.3f%s\n"," /height ", height, " def");
	fprintf( outfile, "%s%9.3f%s\n"," /width ", width, " def");
	fprintf( outfile, "%s%9.3f%s\n"," /xPos ", pagepos[0], " def");
	fprintf( outfile, "%s%9.3f%s\n"," /yPos ", pagepos[1], " def");
	fprintf( outfile, "%s%d%s\n",	" /fontpt ", fontpoint, " def");

/*
 *  Postscript buffer for image line
 */

	if (numinp == 1)  /* grayscale image */
	{
		fprintf( outfile, "%s%d%s\n",	" /picstr ", nso, " string def");
	}
	else if (numinp==3)
	{
		fprintf( outfile, "%s%d%s\n",	" /Rstr ", nso, " string def");
		fprintf( outfile, "%s%d%s\n",	" /Gstr ", nso, " string def");
		fprintf( outfile, "%s%d%s\n",	" /Bstr ", nso, " string def");
	}
	else
	{
		zvmessage( "*** Specify one or 3 images, only ***", "");
		zabend();
	}

/*
 *  Define PostScript procedure to read in hex image data
 */

	fprintf( outfile, "%s\n",	" /vicarimage");
	fprintf( outfile, " { %d %d %d [ %d 0 0 %d 0 %d ] \n",
				nso,nlo,nbits,nso,-nlo,nlo );
	if (numinp == 1)
	{
		fprintf( outfile, "     {currentfile picstr readhexstring pop}\n");
		fprintf( outfile, "     image\n");
		strcpy(command_part, "lpr ");
	}
	else
	{
		fprintf( outfile, "     {currentfile Rstr readhexstring pop}\n");
		fprintf( outfile, "     {currentfile Gstr readhexstring pop}\n");
		fprintf( outfile, "     {currentfile Bstr readhexstring pop}\n");
		fprintf( outfile, "     true 3\n");
		fprintf( outfile, "     colorimage\n");
		strcpy(command_part, "lp -dphaser183-518 ");
	}
	fprintf( outfile, "     }  def\n");

/*
 * Save old POSTSCRIPT Parameters and change scaling,position:
 */

	fprintf( outfile, " gsave\n");
	zvparm( "freq", &freq, &count, &def, 0,0);
	if (def==0) fprintf( outfile, " %9.2f %s\n", freq,
		"45 {dup mul exch  dup mul add 1 exch sub} setscreen" );
	
	if (zvptst( "wide" )) 
		fprintf( outfile, "  0 %6.2f inch  translate -90 rotate\n", 
					pagesize[0] );
		
	fprintf( outfile, " xPos inch yPos inch translate\n");

	if (zvptst( "wide" )) 
		fprintf( outfile, "%s\n", " gsave %Save for text");

	fprintf( outfile, " width inch height inch scale\n");

 
	fprintf( outfile, " vicarimage\n");	/* Call image reading  procedure */

/*
 * Read in VICAR image and convert to Hexadecimal text output
 */

	for (line=0; line<nlo; line++)
	{
	   for (band=0; band<numinp; band++)
	   {
		ptr=lbuff;
		zvread( inunit[band], lbuff, 
			"line", line*increment + 1, NULL );
		if (scale)
			scale_image( lbuff, slope,offset,ns,increment);
		else if (increment>1)
			subsample_image(lbuff, ns, increment);
		
		/* write out in blocks of <hexcount> values */
		
		for (samp=0; samp<nsb; samp++)
		{
		   for (i=0; i<hexcount; i++)
		   fprintf( outfile, hexformat, (*ptr++)>>bitshift );
		   fprintf( outfile, "\n" );
		}
		
		/* write out the rest of line */
		
		endptr = lbuff+nso;
		while (ptr < endptr)
		fprintf( outfile, hexformat, (*ptr++)>>bitshift );
		fprintf( outfile, "\n" );
		
	    } /* band loop */
	} /* line  loop */
	
	/*** end of image-dump ***/
	
	fprintf( outfile, " grestore\n" );  /* restores page parameters */
	
	if (dotitle)
	{
	    /* font name and scale */
	   
	    fprintf( outfile, "  /%s findfont fontpt scalefont\n", fontname );
	    fprintf( outfile, "  setfont\n");
	    
	    /***
	     *** Next, position the TITLE centered beneath the image
	     *** by use of the formulas, translated from the Reverse Polish:
	     ***           X_position = POS(1) + (WIDTH - STR_WIDTH)/2
	     ***           Y_position = POS(2) - (MARGIN + STR_HEIGHT)
	     ***/

	    fprintf( outfile, "  %% The following centers title below image:\n");

            if (numtitles == 0)
            {
                strcpy(title[0],filename);
		numtitles = 1;
	    }
	    
	    if (zvptst( "wide" )) 
	    {
	        for (i=0; i<numtitles; i++)
	        {
	            fprintf( outfile, "  (%s)\n", title[i] );
		    fprintf( outfile, " dup stringwidth pop\n");
		    fprintf( outfile, " width inch exch sub 2 div\n");
		    fprintf( outfile, " %d fontpt mul -1 mul\n", i+1);
		    fprintf( outfile, " %6.3f inch sub moveto show\n", margin);
		}
	    }
	    else
	    {
	        for (i=0; i<numtitles; i++)
	        {
	            fprintf( outfile, "  (%s)\n", title[i] );
		    fprintf( outfile,
		      " dup stringwidth pop %d fontpt mul %9.3f inch add\n",
				   i+1,  margin);
		    fprintf( outfile,
		      " yPos inch exch sub exch %s\n", "% Y-Position on page");
		    fprintf( outfile,
		     " width inch sub 2 div %s\n", "% Width of image in inches");
		    fprintf( outfile,
	 		" xPos inch exch sub exch moveto show %s\n",
			"% X-position");
		}
	    }

	    fprintf( outfile, " grestore  %s\n", "% Restore former parameters");

	} /* title */
	
	fprintf( outfile, " showpage %s\n", " % Command to print out page.");
	
	fclose( outfile );
	for (i=0; i<numinp; i++)
	   zvclose( inunit[i], NULL);
                                                     /* added by rea 8/5/93  */
        if (!zvptst("NOPRINT"))
        {
           strcpy(command_line,command_part);
           strcat(command_line,outname);
           system(command_line);
        }

        if (zvptst("PRINT"))
        {
	   system("lpq");
           strcpy(command_line,"rm -f ");
           strcat(command_line,outname);
           system(command_line);
        }
                                                    /* end of 8/5/93 addition */
}



/* subsample a byte array (no scaling) */

subsample_image(lbuf, ns, increment)
unsigned char *lbuf;	/* input/output -- the values to scale	*/
int ns;			/* input - number of values		*/
int increment;		/* input - subsampling			*/
{
	register unsigned char *buf = lbuf;
	register int i;
	
	for (i=0; i<ns; i+=increment)
		(*lbuf++) = buf[i];
}



/* scale and convert a real buffer to byte, and sub-sample */

scale_image( lbuf, slope,offset,ns,increment)
unsigned char *lbuf;	/* input/output -- the values to scale	*/
float slope;		/* input - scale factor 		*/
float offset;		/* input - offset			*/
int ns;			/* input - number of values		*/
int increment;		/* input - subsampling			*/
{
	float *rbuf = (float *)lbuf;
	int value,i;
	
	for (i=0; i<ns; i+=increment)
	{
		/* scale and clip to byte range */

		value = (rbuf[i] - offset) * slope;
		value = value <   0 ?   0 : value;
		(*lbuf++) =  value > 255 ? 255 : value;
	}
}

