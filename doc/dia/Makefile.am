# $Id$

dist_noinst_DATA = bootes.pdf			\
      central-server-messages.pdf 	\
      client-server-direct.pdf 		\
      client-server-central.pdf 	\
      client-server-central-direct.pdf 	\
      client-server-use1.pdf		\
      client-server-use2.pdf		\
      client-server-use3.pdf		\
      db.pdf				\
      db-overview.pdf			\
      kill-nextexe.pdf 			\
      kill-pre.pdf 			\
      kill-waitexe.pdf 			\
      kill-waitblock.pdf 		\
      overview.pdf			\
      overview-en.pdf			\
      state-serverd.pdf			\
      state-camd.pdf			\
      state-cooling.pdf			\
      state-teld.pdf			\
      state-dome.pdf			\
      rts-1-packages.pdf 		\
      vrstvy-architektura.pdf 

%.eps: %.dia
	dia --nosplash --export=$@ $<

%.pdf: %.eps
	epstopdf $<
