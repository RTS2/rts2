struct target
{
	int		id;
	double		ra;
	double		dec;
	time_t		ctime;
	struct target*	next;
};

int make_plan (struct target **plan);
void free_plan (struct target *plan);
