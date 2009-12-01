#include "engine.h"

Texture *sky[6] = { 0, 0, 0, 0, 0, 0 }, *clouds[6] = { 0, 0, 0, 0, 0, 0 };

void loadsky(const char *basename, Texture *texs[6])
{
    loopi(6)
    {
        const char *side = cubemapsides[i].name;
        defformatstring(name)("%s_%s.jpg", makerelpath("packages", basename), side);
        if((texs[i] = textureload(name, 3, true, false))==notexture)
        {
            strcpy(name+strlen(name)-3, "png");
            if((texs[i] = textureload(name, 3, true, false))==notexture) conoutf(CON_ERROR, "could not load sky texture packages/%s_%s", basename, side);
        }
    }
}

Texture *cloudoverlay = NULL;

Texture *loadskyoverlay(const char *basename)
{
    defformatstring(name)("%s.jpg", makerelpath("packages", basename));
    Texture *t = textureload(name, 0, true, false);
    if(t!=notexture) return t;
    strcpy(name+strlen(name)-3, "png");
    t = textureload(name, 0, true, false);
    if(t==notexture) conoutf(CON_ERROR, "could not load sky overlay texture packages/%s", basename);
    return t;
}

SVARFR(skybox, "", { if(skybox[0]) loadsky(skybox, sky); }); 
FVARR(spinsky, -720, 0, 720);
VARR(yawsky, 0, 0, 360);
SVARFR(cloudbox, "", { if(cloudbox[0]) loadsky(cloudbox, clouds); });
FVARR(spinclouds, -720, 0, 720);
VARR(yawclouds, 0, 0, 360);
FVARR(cloudclip, 0, 0.5f, 1);
SVARFR(cloudlayer, "", { if(cloudlayer[0]) cloudoverlay = loadskyoverlay(cloudlayer); });
FVARR(cloudscrollx, -16, 0, 16);
FVARR(cloudscrolly, -16, 0, 16);
FVARR(cloudscale, 0.001, 1, 64);
FVARR(spincloudlayer, -720, 0, 720);
VARR(yawcloudlayer, 0, 0, 360);
FVARR(cloudheight, -1, 0.2f, 1);
FVARR(cloudfade, 0, 0.2f, 1);
FVARR(cloudalpha, 0, 1, 1);
VARR(cloudsubdiv, 4, 16, 64);
HVARR(cloudcolour, 0, 0xFFFFFF, 0xFFFFFF);

void draw_envbox_face(float s0, float t0, int x0, int y0, int z0,
                      float s1, float t1, int x1, int y1, int z1,
                      float s2, float t2, int x2, int y2, int z2,
                      float s3, float t3, int x3, int y3, int z3,
                      GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS);
    glTexCoord2f(s3, t3); glVertex3f(x3, y3, z3);
    glTexCoord2f(s2, t2); glVertex3f(x2, y2, z2);
    glTexCoord2f(s1, t1); glVertex3f(x1, y1, z1);
    glTexCoord2f(s0, t0); glVertex3f(x0, y0, z0);
    glEnd();
    xtraverts += 4;
}

void draw_envbox(int w, float zclip = 0.0f, int faces = 0x3F, Texture **sky = NULL)
{
    float vclip = 1-zclip;
    int z = int(ceil(2*w*(vclip-0.5f)));

    if(faces&0x01)
        draw_envbox_face(0.0f, 0.0f, -w, -w, -w,
                         1.0f, 0.0f, -w,  w, -w,
                         1.0f, vclip, -w,  w,  z,
                         0.0f, vclip, -w, -w,  z, sky[0] ? sky[0]->id : notexture->id);

    if(faces&0x02)
        draw_envbox_face(1.0f, vclip, +w, -w,  z,
                         0.0f, vclip, +w,  w,  z,
                         0.0f, 0.0f, +w,  w, -w,
                         1.0f, 0.0f, +w, -w, -w, sky[1] ? sky[1]->id : notexture->id);

    if(faces&0x04)
        draw_envbox_face(1.0f, vclip, -w, -w,  z,
                         0.0f, vclip,  w, -w,  z,
                         0.0f, 0.0f,  w, -w, -w,
                         1.0f, 0.0f, -w, -w, -w, sky[2] ? sky[2]->id : notexture->id);

    if(faces&0x08)
        draw_envbox_face(1.0f, vclip, +w,  w,  z,
                         0.0f, vclip, -w,  w,  z,
                         0.0f, 0.0f, -w,  w, -w,
                         1.0f, 0.0f, +w,  w, -w, sky[3] ? sky[3]->id : notexture->id);

    if(!zclip && faces&0x10)
        draw_envbox_face(0.0f, 1.0f, -w,  w,  w,
                         0.0f, 0.0f, +w,  w,  w,
                         1.0f, 0.0f, +w, -w,  w,
                         1.0f, 1.0f, -w, -w,  w, sky[4] ? sky[4]->id : notexture->id);

    if(faces&0x20)
        draw_envbox_face(0.0f, 1.0f, +w,  w, -w,
                         0.0f, 0.0f, -w,  w, -w,
                         1.0f, 0.0f, -w, -w, -w,
                         1.0f, 1.0f, +w, -w, -w, sky[5] ? sky[5]->id : notexture->id);
}

void draw_env_overlay(int w, Texture *overlay = NULL, float tx = 0, float ty = 0)
{
    float z = -w*cloudheight, tsz = 0.5f*(1-cloudfade)/cloudscale, psz = w*(1-cloudfade);
    glBindTexture(GL_TEXTURE_2D, overlay ? overlay->id : notexture->id);
    float r = (cloudcolour>>16)/255.0f, g = ((cloudcolour>>8)&255)/255.0f, b = (cloudcolour&255)/255.0f;
    glColor4f(r, g, b, cloudalpha);
    glBegin(GL_POLYGON);
    loopi(cloudsubdiv)
    {
        vec p(1, 1, 0);
        p.rotate_around_z((-2.0f*M_PI*i)/cloudsubdiv);
        glTexCoord2f(tx + p.x*tsz, ty + p.y*tsz); glVertex3f(p.x*psz, p.y*psz, z);
    }
    glEnd();
    float tsz2 = 0.5f/cloudscale;
    glBegin(GL_QUAD_STRIP);
    loopi(cloudsubdiv+1)
    {
        vec p(1, 1, 0);
        p.rotate_around_z((-2.0f*M_PI*i)/cloudsubdiv);
        glColor4f(r, g, b, cloudalpha);
        glTexCoord2f(tx + p.x*tsz, ty + p.y*tsz); glVertex3f(p.x*psz, p.y*psz, z);
        glColor4f(r, g, b, 0);
        glTexCoord2f(tx + p.x*tsz2, ty + p.y*tsz2); glVertex3f(p.x*w, p.y*w, z);
    }
    glEnd();    
}

static struct domevert
{
    vec pos;
    uchar color[4];

	domevert() {}
	domevert(const vec &pos, float alpha) : pos(pos)
	{
		memcpy(color, fogcolor.v, 3);
		color[3] = uchar(alpha*255);
	}
	domevert(const domevert &v0, const domevert &v1) : pos(vec(v0.pos).add(v1.pos).normalize())
	{
        memcpy(color, v0.color, 4);
        if(v0.pos.z != v1.pos.z) color[3] += uchar((v1.color[3] - v0.color[3]) * (pos.z - v0.pos.z) / (v1.pos.z - v0.pos.z));
	}
} *domeverts = NULL;
static GLushort *domeindices = NULL;
static int domenumverts = 0, domenumindices = 0, domecapindices = 0;
static GLuint domevbuf = 0, domeebuf = 0;
static bvec domecolor(0, 0, 0);
static float domeminalpha = 0, domemaxalpha = 0;

static void subdivide(int depth, int face);

static void genface(int depth, int i1, int i2, int i3)
{
    int face = domenumindices; domenumindices += 3;
    domeindices[face]   = i3;
    domeindices[face+1] = i2;
    domeindices[face+2] = i1;
    subdivide(depth, face);
}

static void subdivide(int depth, int face)
{
    if(depth-- <= 0) return;
    int idx[6];
    loopi(3) idx[i] = domeindices[face+2-i];
    loopi(3)
    {
        int vert = domenumverts++;
        domeverts[vert] = domevert(domeverts[idx[i]], domeverts[idx[(i+1)%3]]); //push on to unit sphere
        idx[3+i] = vert;
        domeindices[face+2-i] = vert;
    }
    subdivide(depth, face);
    loopi(3) genface(depth, idx[i], idx[3+i], idx[3+(i+2)%3]);
}

static int sortdomecap(const GLushort *x, const GLushort *y)
{
    const vec &xv = domeverts[*x].pos, &yv = domeverts[*y].pos;
    if(xv.y < 0)
    {
        if(yv.y >= 0 || xv.x < yv.x) return 1;
        if(xv.x > yv.x) return -1;
    }
    else if(yv.y < 0 || xv.x < yv.x) return -1;
    else if(xv.x > yv.x) return 1;
    return 0;
}

static void initdome(float minalpha = 0.0f, float maxalpha = 1.0f, int hres = 16, int depth = 2)
{
    const int tris = hres << (2*depth);
    domenumverts = domenumindices = 0;
    DELETEA(domeverts);
    DELETEA(domeindices);
    domeverts = new domevert[tris+1];
    domeindices = new GLushort[(tris + ((hres<<depth)-2))*3];
	domeverts[domenumverts++] = domevert(vec(0.0f, 0.0f, 1.0f), minalpha); //build initial 'hres' sided pyramid
    loopi(hres)
    {
        float angle = 2*M_PI*float(i)/hres;
        domeverts[domenumverts++] = domevert(vec(cosf(angle), sinf(angle), 0.0f), maxalpha);
    }
    loopi(hres) genface(depth, 0, i+1, 1+(i+1)%hres);

    GLushort *domecap = &domeindices[domenumindices];
    int domecapverts = 0;
    loopi(domenumverts) if(!domeverts[i].pos.z) domecap[domecapverts++] = i;
    quicksort(domecap, domecapverts, sortdomecap); 
    domecapindices = 3;
    loopi(domecapverts-3)
    {
        int n = domecapverts-3-i;
        domecap[n*3] = domecap[n+2];
        domecap[n*3+1] = domecap[n+1];
        domecap[n*3+2] = domecap[0];
        domecapindices += 3;
    }
    swap(domecap[0], domecap[2]);

    if(hasVBO)
    {
        if(!domevbuf) glGenBuffers_(1, &domevbuf);
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, domevbuf);
        glBufferData_(GL_ARRAY_BUFFER_ARB, domenumverts*sizeof(domevert), domeverts, GL_STATIC_DRAW_ARB);
        DELETEA(domeverts);

        if(!domeebuf) glGenBuffers_(1, &domeebuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, domeebuf);
        glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, (domenumindices + domecapindices)*sizeof(GLushort), domeindices, GL_STATIC_DRAW_ARB);
        DELETEA(domeindices);
    }
}

static void deletedome()
{
	domenumverts = domenumindices = 0;
    if(domevbuf) { glDeleteBuffers_(1, &domevbuf); domevbuf = 0; }
    if(domeebuf) { glDeleteBuffers_(1, &domeebuf); domeebuf = 0; }
    DELETEA(domeverts);
    DELETEA(domeindices);
}

FVARR(fogdomeheight, -1, -0.05f, 1); 
FVARR(fogdomemin, 0, 0, 1);
FVARR(fogdomemax, 0, 0, 1);
VARR(fogdomecap, 0, 0, 1);

static void drawdome()
{
	if(!domenumverts || domecolor != fogcolor || domeminalpha != fogdomemin || domemaxalpha != fogdomemax) 
	{
		initdome(min(fogdomemin, fogdomemax), fogdomemax);
		domecolor = fogcolor;
		domeminalpha = fogdomemin;
		domemaxalpha = fogdomemax;
	}

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, domevbuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, domeebuf);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(domevert), &domeverts->pos);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(domevert), &domeverts->color);

    if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, 0, domenumverts-1, domenumindices + fogdomecap*domecapindices, GL_UNSIGNED_SHORT, domeindices);
    else glDrawElements(GL_TRIANGLES, domenumindices + fogdomecap*domecapindices, GL_UNSIGNED_SHORT, domeindices);
    xtraverts += domenumverts;
    glde++;

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
}

void cleanupsky()
{
    deletedome();
}

VARP(sparklyfix, 0, 0, 1);
VAR(showsky, 0, 1, 1); 
VAR(clipsky, 0, 1, 1);

bool drawskylimits(bool explicitonly)
{
    nocolorshader->set();

    glDisable(GL_TEXTURE_2D);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    bool rendered = rendersky(explicitonly);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_TEXTURE_2D);

    return rendered;
}

void drawskyoutline()
{
    notextureshader->set();

    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_FALSE);
    extern int wireframe;
    if(!wireframe)
    {
        enablepolygonoffset(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    glColor3f(0.5f, 0.0f, 0.5f);
    rendersky(true);
    if(!wireframe) 
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        disablepolygonoffset(GL_POLYGON_OFFSET_LINE);
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_TEXTURE_2D);

    if(!glaring) defaultshader->set();
}

VAR(clampsky, 0, 1, 1);

static int yawskyfaces(int faces, int yaw, float spin = 0)
{
    if(spin || yaw%90) return faces&0x0F ? faces | 0x0F : faces;
    static const int faceidxs[3][4] =
    {
        { 3, 2, 0, 1 },
        { 1, 0, 3, 2 },
        { 2, 3, 1, 0 }
    };
    yaw /= 90;
    if(yaw < 1 || yaw > 3) return faces;
    const int *idxs = faceidxs[yaw - 1];
    return (faces & ~0x0F) | (((faces>>idxs[0])&1)<<0) | (((faces>>idxs[1])&1)<<1) | (((faces>>idxs[2])&1)<<2) | (((faces>>idxs[3])&1)<<3);
}

void drawskybox(int farplane, bool limited)
{
    extern int renderedskyfaces, renderedskyclip; // , renderedsky, renderedexplicitsky;
    bool alwaysrender = editmode || !insideworld(camera1->o) || reflecting,
         explicitonly = false;
    if(limited)
    {
        explicitonly = alwaysrender || !sparklyfix || refracting; 
        if(!drawskylimits(explicitonly) && !alwaysrender) return;
        extern int ati_skybox_bug;
        if(!alwaysrender && !renderedskyfaces && !ati_skybox_bug) explicitonly = false;
    }
    else if(!alwaysrender)
    {
        extern vtxarray *visibleva;
        renderedskyfaces = 0;
        renderedskyclip = INT_MAX;
        for(vtxarray *va = visibleva; va; va = va->next)
        {
            if(va->occluded >= OCCLUDE_BB && va->skyfaces&0x80) continue;
            renderedskyfaces |= va->skyfaces&0x3F;
            if(!(va->skyfaces&0x1F) || camera1->o.z < va->skyclip) renderedskyclip = min(renderedskyclip, va->skyclip);
            else renderedskyclip = 0;
        }
        if(!renderedskyfaces) return;
    }
    
    if(alwaysrender)
    {
        renderedskyfaces = 0x3F;
        renderedskyclip = 0;
    }

    float skyclip = clipsky ? max(renderedskyclip-1, 0) : 0;
    if(reflectz<worldsize && reflectz>skyclip) skyclip = reflectz;

    if(glaring)
    {
        static Shader *skyboxglareshader = NULL;
        if(!skyboxglareshader) skyboxglareshader = lookupshaderbyname("skyboxglare");
        skyboxglareshader->set();
    }
    else defaultshader->set();

    if(renderpath!=R_FIXEDFUNCTION || !fogging) glDisable(GL_FOG);

    if(limited) 
    {
        if(explicitonly) glDisable(GL_DEPTH_TEST);
        else glDepthFunc(GL_GEQUAL);
    }
    else glDepthFunc(GL_LEQUAL);

    glDepthMask(GL_FALSE);

    if(clampsky) glDepthRange(1, 1);

    glColor3f(1, 1, 1);

    glPushMatrix();
    glLoadIdentity();
    glRotatef(camera1->roll, 0, 0, 1);
    glRotatef(camera1->pitch, -1, 0, 0);
    glRotatef(camera1->yaw+spinsky*lastmillis/1000.0f+yawsky, 0, 1, 0);
    glRotatef(90, 1, 0, 0);
    if(reflecting) glScalef(1, 1, -1);
    draw_envbox(farplane/2, skyclip ? 0.5f + 0.5f*(skyclip-camera1->o.z)/float(worldsize) : 0, yawskyfaces(renderedskyfaces, yawsky, spinsky), sky);
    glPopMatrix();

    if(!glaring && cloudbox[0])
    {
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPushMatrix();
        glLoadIdentity();
        glRotatef(camera1->roll, 0, 0, 1);
        glRotatef(camera1->pitch, -1, 0, 0);
        glRotatef(camera1->yaw+spinclouds*lastmillis/1000.0f+yawclouds, 0, 1, 0);
        glRotatef(90, 1, 0, 0);
        if(reflecting) glScalef(1, 1, -1);
        draw_envbox(farplane/2, skyclip ? 0.5f + 0.5f*(skyclip-camera1->o.z)/float(worldsize) : cloudclip, yawskyfaces(renderedskyfaces, yawclouds, spinclouds), clouds);
        glPopMatrix();

        glDisable(GL_BLEND);
    }

    if(!glaring && cloudlayer[0] && cloudheight && renderedskyfaces&(cloudheight < 0 ? 0x1F : 0x2F))
    {
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

        glDisable(GL_CULL_FACE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPushMatrix();
        glLoadIdentity();
        glRotatef(camera1->roll, 0, 0, 1);
        glRotatef(camera1->pitch, -1, 0, 0);
        glRotatef(camera1->yaw+spincloudlayer*lastmillis/1000.0f+yawcloudlayer, 0, 1, 0);
        glRotatef(90, 1, 0, 0);
        if(reflecting) glScalef(1, 1, -1);
        draw_env_overlay(farplane/2, cloudoverlay, cloudscrollx * lastmillis/1000.0f, cloudscrolly * lastmillis/1000.0f);
        glPopMatrix();

        glDisable(GL_BLEND);

        glEnable(GL_CULL_FACE);
    }

	if(!glaring && fogdomemax)
	{
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

        notextureshader->set();
        glDisable(GL_TEXTURE_2D);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPushMatrix();
        glLoadIdentity();
        glRotatef(camera1->roll, 0, 0, 1);
        glRotatef(camera1->pitch, -1, 0, 0);
        glRotatef(camera1->yaw, 0, 1, 0);
        glRotatef(90, 1, 0, 0);
        if(reflecting) glScalef(1, 1, -1);
		glTranslatef(0, 0, -farplane*fogdomeheight*0.5f);
		glScalef(farplane/2, farplane/2, -farplane*(0.5f - fogdomeheight*0.5f));
		drawdome();
        glPopMatrix();

        glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
	}

    if(clampsky) glDepthRange(0, 1);

    glDepthMask(GL_TRUE);

    if(limited)
    {
        if(explicitonly) glEnable(GL_DEPTH_TEST);
        else glDepthFunc(GL_LESS);
        if(!reflecting && !refracting && !envmapping && editmode && showsky) drawskyoutline();
    }
    else glDepthFunc(GL_LESS);

    if(renderpath!=R_FIXEDFUNCTION || !fogging) glEnable(GL_FOG);
}

VARNR(skytexture, useskytexture, 0, 1, 1);

int explicitsky = 0;
double skyarea = 0;

bool limitsky()
{
    return (explicitsky && (useskytexture || editmode)) || (sparklyfix && skyarea / (double(worldsize)*double(worldsize)*6) < 0.9);
}

