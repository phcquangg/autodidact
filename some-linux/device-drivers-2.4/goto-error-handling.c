int init_module(void)
{
	int err;
	
	err = register_this(ptr1, "str1");
	if (err) goto failed1;

	err = register_that(ptr2, "str2");
	if (err) goto failed2;

	err = register_those(ptr3, "str3");
	if (err) goto failed3;

	return 0;

	failed1: unregister_1(ptr1, "str1");
	failed2: unregister_2(ptr2, "str2");
	failed3: return err;
}
