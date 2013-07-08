#include <stdio.h>
#include <GL/glut.h>
#include <OpenGL/gl.h>
#include <memory.h>
#include <fcntl.h>
#include <assert.h>
#include <png.h>

#define READ_PORTION_SIZE 4096

#define ATTR_POS 0
#define ATTR_POS_NAME "a_pos"

#define ATTR_TEX_COORD 1
#define ATTR_TEX_COORD_NAME "a_texCrds"

GLint load_shader(const char* fname, GLuint type) {
	void* shader_src = NULL;
	int shader_f = open(fname, O_RDONLY);
	if (!shader_f) return 0;

	size_t shader_src_len = 0;
	size_t bytes_read;
	void* bytes = malloc(READ_PORTION_SIZE);
	while (bytes_read = read(shader_f, bytes, READ_PORTION_SIZE)) {
		if (shader_src_len == 0) {
			shader_src = malloc(bytes_read);
		} else {
			shader_src = realloc(shader_src, shader_src_len + bytes_read);
		}
		memcpy(shader_src + shader_src_len, bytes, bytes_read);
		shader_src_len += bytes_read;
	}
 	free(bytes);
	close(shader_f);

	shader_src = realloc(shader_src, shader_src_len + 1);
	*((char*)shader_src + shader_src_len) = '\0';
	if (!shader_src_len) return 0;

 	GLuint shader_id = glCreateShader(type);
 	assert(glGetError() == GL_NO_ERROR);
	glShaderSource(shader_id, 1, &shader_src, 0);
	assert(glGetError() == GL_NO_ERROR);
	glCompileShader(shader_id);
	assert(glGetError() == GL_NO_ERROR);
	free(shader_src);

/* 	GLboolean status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
  
    if (status == GL_FALSE) {
*/    	
    	int log_size = 0;
    	char log_bytes[2048];
    	glGetShaderInfoLog(shader_id, 2048, &log_size, log_bytes);

    	if (log_size > 0) {
	    	printf("%s shader build failed due to %s\n", type == GL_VERTEX_SHADER ? "vertex" : "fragment", log_bytes);
	    	return 0;
    	}
    // }

    return shader_id;
}

GLuint make_program() {
	GLuint vshader_id = load_shader("vertex.glsl", GL_VERTEX_SHADER);
	if (!vshader_id) return 0;

	GLuint fshader_id = load_shader("fragment.glsl", GL_FRAGMENT_SHADER);
	if (!fshader_id) return 0;

    GLuint prog_id = glCreateProgram();
    glAttachShader(prog_id, vshader_id);
    assert(glGetError() == GL_NO_ERROR);
    glAttachShader(prog_id, fshader_id);
    assert(glGetError() == GL_NO_ERROR);

    glBindAttribLocation(prog_id, ATTR_POS, ATTR_POS_NAME);
    glBindAttribLocation(prog_id, ATTR_TEX_COORD, ATTR_TEX_COORD_NAME);


    glLinkProgram(prog_id);
    assert(glGetError() == GL_NO_ERROR);

    // GLboolean status;
    // glGetProgramiv(prog_id, GL_LINK_STATUS, &status);

    	int log_size = 0;
    	char log_bytes[2048];
    	glGetProgramInfoLog(prog_id, 2048, &log_size, log_bytes);

    	if (log_size > 0) {
	    	printf("program build failed due to %s\n", log_bytes);
	    	// return 0;
    	}

	glDeleteShader(vshader_id);
	glDeleteShader(fshader_id);

    return prog_id;
}

#define CC_RGB_PREMULTIPLY_APLHA(vr, vg, vb, va) \
    (unsigned)(((unsigned)((unsigned char)(vr) * ((unsigned char)(va) + 1)) >> 8) | \
    ((unsigned)((unsigned char)(vg) * ((unsigned char)(va) + 1) >> 8) << 8) | \
    ((unsigned)((unsigned char)(vb) * ((unsigned char)(va) + 1) >> 8) << 16) | \
    ((unsigned)(unsigned char)(va) << 24))

void* load_png_image(int fd) {
	png_structp     png_ptr     =   0; 
	png_infop       info_ptr    = 0;
	unsigned char * pImageData  = 0;

	FILE *fp = fdopen(fd,"rb");

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if( png_ptr == NULL ){
		fprintf(stderr,"can't create png_ptr\n");
		fclose(fp);
		return NULL;
	}
	// init png_info
	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL ){
		fprintf(stderr,"can't create info_ptr\n");
		fclose(fp);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return NULL;
	}

	// set the read call back function
	png_init_io(png_ptr, fp);

	// read png
	// PNG_TRANSFORM_EXPAND: perform set_expand()
	// PNG_TRANSFORM_PACKING: expand 1, 2 and 4-bit samples to bytes
	// PNG_TRANSFORM_STRIP_16: strip 16-bit samples to 8 bits
	// PNG_TRANSFORM_GRAY_TO_RGB: expand grayscale samples to RGB (or GA to RGBA)

	png_read_png(png_ptr, info_ptr, 
			PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_GRAY_TO_RGB, 
			0
	);

	int         color_type  = 0;
	png_uint_32 width = 0;
	png_uint_32 height = 0;
	int         bitsPerComponent = 0;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitsPerComponent, &color_type, 0, 0, 0);

	// init image info
	int preMulti = 1;
	int hasAlpha = ( png_get_color_type(png_ptr, info_ptr) & PNG_COLOR_MASK_ALPHA ) ? 1 : 0;

	// allocate memory and read data
	int bytesPerComponent = 3;
	if (hasAlpha) bytesPerComponent = 4;

	//unsigned int legalWidth = nextPowerOfTwo(width);
	//unsigned int legalHeight = nextPowerOfTwo(height);
	unsigned int legalWidth = width;
	unsigned int legalHeight = height;

	unsigned int dataLen = legalHeight * legalWidth * bytesPerComponent;

	pImageData = malloc(dataLen);

	png_bytep * rowPointers = png_get_rows(png_ptr, info_ptr);

	// copy data to image info
	unsigned int bytesPerRow = width * bytesPerComponent;
	unsigned int i,j;


	if(hasAlpha)
	{
		unsigned int *tmp = (unsigned int *)pImageData;
		unsigned int rowDiff = legalWidth - width;
		unsigned char red,green,blue,alpha;
		for(i = 0; i < height; i++)
		{
			for(j = 0; j < bytesPerRow; j += 4)
			{
				red = rowPointers[height - i - 1][j];
				green = rowPointers[height - i - 1][j + 1];
				blue = rowPointers[height - i - 1][j + 2];
				alpha = rowPointers[height - i - 1][j + 3];
				*tmp++ = CC_RGB_PREMULTIPLY_APLHA(red, green, blue, alpha);
			}
			tmp += rowDiff;
		}
	}
	else
	{
		unsigned int bytesPerLegalRow = legalWidth * bytesPerComponent;
		for (j = 0; j < height; ++j)
		{
			memcpy(pImageData + j * bytesPerLegalRow, rowPointers[height - j - 1], bytesPerRow);
		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	fclose(fp);


	return pImageData;
}

GLuint make_tex() {
	int tex_f = open("tree.png", O_RDONLY);
	if (tex_f < 0) return 0;
	void* imgdata = load_png_image(tex_f);

	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgdata);

	return tex_id;
}

typedef struct {
	GLfloat tx, ty;
	GLfloat x, y;
} vertex_t;

void reshape(int w, int h) {
	GLuint prog_id = make_program();
	if (!prog_id) return;

	glUseProgram(prog_id);
	assert(glGetError() == GL_NO_ERROR);

	GLuint src_tex_id = make_tex();
	GLuint u_tex_loc = glGetUniformLocation(prog_id, "u_tex");
	glUniform1i(u_tex_loc, 0);	

	vertex_t vertices[] = { (vertex_t){ 0.0, 0.0, -1.0, -1.0 }, (vertex_t){ 0.0, 1.0, -1.0, 1.0 },
							(vertex_t){ 1.0, 0.0, 1.0, -1.0 }, (vertex_t){ 1.0, 1.0, 1.0, 1.0 } };
	GLuint buf;
	glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(ATTR_POS);
	glEnableVertexAttribArray(ATTR_TEX_COORD);
	glVertexAttribPointer(ATTR_POS, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), sizeof(GLfloat) * 2);
	glVertexAttribPointer(ATTR_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), 0);

	GLuint buf_texs[2];
	glGenTextures(2, buf_texs);

	glViewport(0, 0, 256, 256);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	int i;
	for (i = 0; i < 2; i++) {
		glBindTexture(GL_TEXTURE_2D, buf_texs[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	GLuint fbuf;
	glGenFramebuffers(1, &fbuf);
	glBindFramebuffer(GL_FRAMEBUFFER, fbuf);

	GLuint cur_tex = src_tex_id;
	for (i = 0; i < 3; i++) {
		glBindTexture(GL_TEXTURE_2D, cur_tex);
		cur_tex = buf_texs[i % 2];
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cur_tex, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, cur_tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);	
	glBindTexture(GL_TEXTURE_2D, 0);
	glutSwapBuffers();
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(256, 256);
	glutCreateWindow("qweqweqwe");
	glutReshapeFunc(reshape);
	glutMainLoop();

	return 0;
}