
boost_home = `echo $(BOOST_HOME)`


header_dir = -I../base -I./ -I../deps/tinyxml -I$(boost_home)
lib_dir = -L../deps/tinyxml -L$(boost_home)/stage/lib -L../base
LIBS = -lseal -lmysqlcppconn -lpthread -llog4cxx -lboost_system -lboost_thread -lboost_chrono -ltinyxml

LIBSEAL= ../base/libseal.a




#-Wall
CXXFLAGS = $(SEAL_COMPILE_OPT)  -D_GNU_SOURCE -D_REENTRANT $(header_dir)
CXXLFLAGS = $(SEAL_COMPILE_OPT) -Wall $(lib_dir) $(LIBS)

objects = SuperServer.o ServerTask.o ServerManager.o FLClient.o

SRCS = $(objects:%.o=%.cpp)
DEPS = $(objects:%.o=.%.d)

ALL_TARGETS = SuperServer

all: $(ALL_TARGETS)

include $(DEPS)
$(DEPS): .%.d: %.cpp
	@$(CXX) -M $(CXXFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

$(objects): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(ALL_TARGETS): $(objects) $(LIBSEAL)
	$(CXX) $(objects) -o $@ $(CXXLFLAGS)

clean:
	@rm -rf *.o .*.d* $(ALL_TARGETS) core *~
