/*
 * (c) copyright 1983 by the Vrije Universiteit, Amsterdam, The Netherlands.
 *
 *          This product is part of the Amsterdam Compiler Kit.
 *
 * Permission to use, sell, duplicate or disclose this software must be
 * obtained in writing. Requests for such permissions may be sent to
 *
 *      Dr. Andrew S. Tanenbaum
 *      Wiskundig Seminarium
 *      Vrije Universiteit
 *      Postbox 7161
 *      1007 MC Amsterdam
 *      The Netherlands
 *
 */

/* Author: E.G. Keizer */

main() {
	t1() ;
	return 0 ;
}

t1() {
	char c ; int i ; long l ; unsigned u ; float f ;

	/* test conversions */

	/* first some conversions on constants */

	printf("(int) '\\377' = %d\n",(int) '\377') ;
	printf("(long) -1 = %ld\n",(long) -1 ) ;
	printf("(float) 12 = %f\n",(float) 12 ) ;
	printf("(int) 3.14 = %d\n",(int) 3.14 ) ;
	printf("(int) 32767L = %d\n",(int) 32767L ) ;
	printf("(int) -32768L = %d\n",(int) -32768L ) ;
	printf("(char) 128L = %d\n",(char) 128L) ;
	printf("(char) 0377 = %d\n",(char) 0377 ) ;
	printf("(char) -1 = %d\n",(char) -1 ) ;
	printf("(char) 10000 = %d\n",(char) 10000 ) ;

	/* conversions from characters */
	printf("From character\n") ;
	c = 127 ;
	i=c ;
	l=c ;
	u=c ;
	f=c ;
	printf("\tchar %5d, int %6d, unsigned %6o, long %11ld, float %f\n",c,i,u,l,f) ;
	c = -1 ;
	i=c ;
	l=c ;
	u=c ;
	f=c ;
	printf("\tchar %5d, int %6d, unsigned %6o, long %11ld, float %f\n",c,i,u,l,f) ;
	c = 0377 ;
	i=c ;
	l=c ;
	u=c ;
	f=c ;
	printf("\tchar %5d, int %6d, unsigned %6o, long %11ld, float %f\n",c,i,u,l,f) ;

	/* from integer */
	printf("From integer\n") ;
	i= -64 ;
	c=i ;
	l=i ;
	u=i ;
	f=i ;
	printf("\tchar %5d, int %6d, unsigned %6o, long %11ld, float %f\n",c,i,u,l,f) ;

	/* from long */
	printf("From long\n") ;
	l = -3 ;
	c = l ;
	i = l ;
	u = l ;
	f = l ;
	printf("\tchar %5d, int %6d, unsigned %6o, long %11ld, float %f\n",c,i,u,l,f) ;

	printf("From float\n") ;
	f = 121.5 ;
	c = f ;
	i = f ;
	u = f ;
	l = f ;
	printf("\tchar %5d, int %6d, unsigned %6o, long %11ld, float %f\n",c,i,u,l,f) ;
	f = 1e-4 ;
	c = f ;
	i = f ;
	u = f ;
	l = f ;
	printf("\tchar %5d, int %6d, unsigned %6o, long %11ld, float %f\n",c,i,u,l,f) ;
	f = 3276.6e1 ;
	i = f ;
	u = f ;
	l = f ;
	printf("\tint %6d, unsigned %6o, long %11ld, float %f\n",i,u,l,f) ;
	f = 1223432e3 ;
	l = f ;
	printf("\tlong %11ld, float %f\n",l,f) ;

	/* some special cases */
	{
		int a[4] ;

		l = 3 ; a[3]= -17 ;
		printf("a[l] (l==%ld) %d\n",l,a[l]) ;
		printf("a[3l] %d\n",a[3l] ) ;

	}
	return 0 ;
}
