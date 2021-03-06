/* _llnull.c */

/*
This is a do-nothing routine which will force autoconf/automake to
include this object into libarch.a.  This allows OSes that cannot
handle a 'ar cru libarch.a' without any objects being specified
to build this library, and later on link with it.
*/

void _llnull(void);

void _llnull(void)
{
  return;
}
