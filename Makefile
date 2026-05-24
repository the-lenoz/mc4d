CXX=g++
LDFLAGS=$(shell pkg-config --static --libs glew glfw3) -lpng
CPPFLAGS=$(shell pkg-config --cflags glew glfw3) -g
CXXFLAGS=-Wall -O0

FILES=src/main.o src/tesseract.o src/config.o src/gpuprogram.o src/noise.o src/world.o src/roundworld.o src/project.o src/gl.o src/readpng.o

mc4d: src/shaders.h $(FILES)
	$(CXX) $(CXXFLAGS) $(FILES) -o mc4d $(LDFLAGS)

# Ensure that the shader headers have been built before the main program is compiled
# If we don't do this, then after cleaning the shaders won't exist and building will fail
src/shaders.h: src/vertShader.h src/fragShader.h src/geomShader.h src/blendvertShader.h src/blendfragShader.h src/wirevertShader.h src/wirefragShader.h src/sbvertShader.h src/sbfragShader.h


# Pull in generated dependency information for existing .o files
-include $(FILES:.o=.d)

# Modify the default c++ compiler rule to generate dependency info too
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CXXFLAGS) $(CPPFLAGS) -MT $*.o $*.cpp > $*.d

# Destroy everything we can generate
.PHONY: clean
clean:
	rm -f src/*.o
	rm -f src/*.d
	rm -f src/*Shader.h
	rm -f mc4d

# Run the project
.PHONY: run
run: mc4d
	./mc4d

# Special generation rules for shaders to embed in program
src/vertShader.h: src/vert.glsl Makefile
	echo "char vertGlsl[] = {" > src/vertShader.h
	xxd -i < src/vert.glsl >> src/vertShader.h
	printf ", 0x00\n};\n" >> src/vertShader.h

src/fragShader.h: src/frag.glsl Makefile
	echo "char fragGlsl[] = {" > src/fragShader.h
	xxd -i < src/frag.glsl >> src/fragShader.h
	printf ", 0x00\n};\n" >> src/fragShader.h

src/geomShader.h: src/geom.glsl Makefile
	echo "char geomGlsl[] = {" > src/geomShader.h
	xxd -i < src/geom.glsl >> src/geomShader.h
	printf ", 0x00\n};\n" >> src/geomShader.h

src/blendvertShader.h: src/blendvert.glsl Makefile
	echo "char blendvertGlsl[] = {" > src/blendvertShader.h
	xxd -i < src/blendvert.glsl >> src/blendvertShader.h
	printf ", 0x00\n};\n" >> src/blendvertShader.h

src/blendfragShader.h: src/blendfrag.glsl Makefile
	echo "char blendfragGlsl[] = {" > src/blendfragShader.h
	xxd -i < src/blendfrag.glsl >> src/blendfragShader.h
	printf ", 0x00\n};\n" >> src/blendfragShader.h

src/wirevertShader.h: src/wirevert.glsl Makefile
	echo "char wirevertGlsl[] = {" > src/wirevertShader.h
	xxd -i < src/wirevert.glsl >> src/wirevertShader.h
	printf ", 0x00\n};\n" >> src/wirevertShader.h

src/wirefragShader.h: src/wirefrag.glsl Makefile
	echo "char wirefragGlsl[] = {" > src/wirefragShader.h
	xxd -i < src/wirefrag.glsl >> src/wirefragShader.h
	printf ", 0x00\n};\n" >> src/wirefragShader.h

src/sbvertShader.h: src/sbvert.glsl Makefile
	echo "char sbvertGlsl[] = {" > src/sbvertShader.h
	xxd -i < src/sbvert.glsl >> src/sbvertShader.h
	printf ", 0x00\n};\n" >> src/sbvertShader.h

src/sbfragShader.h: src/sbfrag.glsl Makefile
	echo "char sbfragGlsl[] = {" > src/sbfragShader.h
	xxd -i < src/sbfrag.glsl >> src/sbfragShader.h
	printf ", 0x00\n};\n" >> src/sbfragShader.h
