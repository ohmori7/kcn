sbin_PROGRAMS = kcndbd
kcndbd_SOURCES = kcndb_main.c kcndb_file.c kcndb_db.c kcndb_server.c
kcndbd_LDADD = @KCN_LIBS@ @EVENT_LIBS@
kcndbd_CFLAGS = -pthread @EVENT_CFLAGS@
install-exec-hook:
	$(MKDIR_P) -m 700 @KCN_DB_PATH@
