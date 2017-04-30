/******************************************************************************
 * Projekt - Zaklady pocitacove grafiky - IZG
 * spanel@fit.vutbr.cz
 *
 * $Id:$
 */

#include "student.h"
#include "transform.h"
#include "fragment.h"

#include <memory.h>
#include <math.h>


/*****************************************************************************
 * Globalni promenne a konstanty
 */

/* Typ/ID rendereru (nemenit) */
const int           STUDENT_RENDERER = 1;
/* Timer */
float				TIME;


/*****************************************************************************
 * Funkce vytvori vas renderer a nainicializuje jej
 */

S_Renderer * studrenCreate()
{
    S_StudentRenderer * renderer = (S_StudentRenderer *)malloc(sizeof(S_StudentRenderer));
    IZG_CHECK(renderer, "Cannot allocate enough memory");

    /* inicializace default rendereru */
    renderer->base.type = STUDENT_RENDERER;
    renInit(&renderer->base);

    /* nastaveni ukazatelu na upravene funkce */
    /* napr. renderer->base.releaseFunc = studrenRelease; */
	renderer->base.releaseFunc = studrenRelease;
	renderer->base.projectTriangleFunc = studrenProjectTriangle;

    /* inicializace nove pridanych casti */
	renderer->color = loadBitmap(TEXTURE_FILENAME, &renderer->w, &renderer->h);

    return (S_Renderer *)renderer;
}

/*****************************************************************************
 * Funkce korektne zrusi renderer a uvolni pamet
 */

void studrenRelease(S_Renderer **ppRenderer)
{
    S_StudentRenderer * renderer;

    if( ppRenderer && *ppRenderer )
    {
        /* ukazatel na studentsky renderer */
        renderer = (S_StudentRenderer *)(*ppRenderer);

        /* pripadne uvolneni pameti */
		free(renderer->color);
        
        /* fce default rendereru */
        renRelease(ppRenderer);
    }
}

/******************************************************************************
 * Nova fce pro rasterizaci trojuhelniku s podporou texturovani
 * Upravte tak, aby se trojuhelnik kreslil s texturami
 * (doplnte i potrebne parametry funkce - texturovaci souradnice, ...)
 * v1, v2, v3 - ukazatele na vrcholy trojuhelniku ve 3D pred projekci
 * n1, n2, n3 - ukazatele na normaly ve vrcholech ve 3D pred projekci
 * x1, y1, ... - vrcholy trojuhelniku po projekci do roviny obrazovky
 */

void studrenDrawTriangle(S_Renderer *pRenderer,
                         S_Coords *v1, S_Coords *v2, S_Coords *v3,
                         S_Coords *n1, S_Coords *n2, S_Coords *n3,
                         int x1, int y1,
                         int x2, int y2,
                         int x3, int y3,
						 S_Coords *t, // koordinaty pre texturu
					     double h1, double h2, double h3 // homogoenna stvrta suradnica 
                         )

{
	int         minx, miny, maxx, maxy;
	int         a1, a2, a3, b1, b2, b3, c1, c2, c3;
	int         s1, s2, s3;
	int         x, y, e1, e2, e3;
	double      alpha, beta, gamma, w1, w2, w3, z;
	S_RGBA      col1, col2, col3, color;

	IZG_ASSERT(pRenderer && v1 && v2 && v3 && n1 && n2 && n3);

	/* vypocet barev ve vrcholech */
	col1 = pRenderer->calcReflectanceFunc(pRenderer, v1, n1);
	col2 = pRenderer->calcReflectanceFunc(pRenderer, v2, n2);
	col3 = pRenderer->calcReflectanceFunc(pRenderer, v3, n3);

	/* obalka trojuhleniku */
	minx = MIN(x1, MIN(x2, x3));
	maxx = MAX(x1, MAX(x2, x3));
	miny = MIN(y1, MIN(y2, y3));
	maxy = MAX(y1, MAX(y2, y3));

	/* oriznuti podle rozmeru okna */
	miny = MAX(miny, 0);
	maxy = MIN(maxy, pRenderer->frame_h - 1);
	minx = MAX(minx, 0);
	maxx = MIN(maxx, pRenderer->frame_w - 1);

	/* Pineduv alg. rasterizace troj.
	hranova fce je obecna rovnice primky Ax + By + C = 0
	primku prochazejici body (x1, y1) a (x2, y2) urcime jako
	(y1 - y2)x + (x2 - x1)y + x1y2 - x2y1 = 0 */

	/* normala primek - vektor kolmy k vektoru mezi dvema vrcholy, tedy (-dy, dx) */
	a1 = y1 - y2;
	a2 = y2 - y3;
	a3 = y3 - y1;
	b1 = x2 - x1;
	b2 = x3 - x2;
	b3 = x1 - x3;

	/* koeficient C */
	c1 = x1 * y2 - x2 * y1;
	c2 = x2 * y3 - x3 * y2;
	c3 = x3 * y1 - x1 * y3;

	/* vypocet hranove fce (vzdalenost od primky) pro protejsi body */
	s1 = a1 * x3 + b1 * y3 + c1;
	s2 = a2 * x1 + b2 * y1 + c2;
	s3 = a3 * x2 + b3 * y2 + c3;

	if (!s1 || !s2 || !s3)
	{
		return;
	}

	/* normalizace, aby vzdalenost od primky byla kladna uvnitr trojuhelniku */
	if (s1 < 0)
	{
		a1 *= -1;
		b1 *= -1;
		c1 *= -1;
	}
	if (s2 < 0)
	{
		a2 *= -1;
		b2 *= -1;
		c2 *= -1;
	}
	if (s3 < 0)
	{
		a3 *= -1;
		b3 *= -1;
		c3 *= -1;
	}

	/* koeficienty pro barycentricke souradnice */
	alpha = 1.0 / ABS(s2);
	beta = 1.0 / ABS(s3);
	gamma = 1.0 / ABS(s1);

	/* vyplnovani... */
	for (y = miny; y <= maxy; ++y)
	{
		/* inicilizace hranove fce v bode (minx, y) */
		e1 = a1 * minx + b1 * y + c1;
		e2 = a2 * minx + b2 * y + c2;
		e3 = a3 * minx + b3 * y + c3;

		for (x = minx; x <= maxx; ++x)
		{
			if (e1 >= 0 && e2 >= 0 && e3 >= 0)
			{
				/* interpolace pomoci barycentrickych souradnic
				e1, e2, e3 je aktualni vzdalenost bodu (x, y) od primek */
				w1 = alpha * e2;
				w2 = beta * e3;
				w3 = gamma * e1;

				/* interpolace z-souradnice */
				z = w1 * v1->z + w2 * v2->z + w3 * v3->z;
				
				/* interpolace textury */
				double h12 = h1 * h2;
				double h23 = h2 * h3;
				double h13 = h1 * h3;
				double denum = h23*w1 + h13*w2 + h12*w3;
				h23 *= w1;
				h13 *= w2;
				h12 *= w3;
				double u = (h23*t[0].x + h13*t[1].x + h12*t[2].x) / denum;
				double v = (h23*t[0].y + h13*t[1].y + h12*t[2].y) / denum;


				/* Ziskanie farby */
				S_RGBA textureColor = studrenTextureValue((S_StudentRenderer *)pRenderer, u, v);

				/* interpolace barvy */
				color.red = ROUND2BYTE(w1 * col1.red + w2 * col2.red + w3 * col3.red);
				color.red = ROUND2BYTE(color.red * textureColor.red / 255.0);
				color.green = ROUND2BYTE(w1 * col1.green + w2 * col2.green + w3 * col3.green);
				color.green = ROUND2BYTE(color.green * textureColor.green / 255.0);
				color.blue = ROUND2BYTE(w1 * col1.blue + w2 * col2.blue + w3 * col3.blue);
				color.blue = ROUND2BYTE(color.blue * textureColor.blue / 255.0);
				color.alpha = 255;

				/* vykresleni bodu */
				if (z < DEPTH(pRenderer, x, y))
				{
					PIXEL(pRenderer, x, y) = color;
					DEPTH(pRenderer, x, y) = z;
				}
			}

			/* hranova fce o pixel vedle */
			e1 += a1;
			e2 += a2;
			e3 += a3;
		}
	}
}

/******************************************************************************
 * Vykresli i-ty trojuhelnik n-teho klicoveho snimku modelu
 * pomoci nove fce studrenDrawTriangle()
 * Pred vykreslenim aplikuje na vrcholy a normaly trojuhelniku
 * aktualne nastavene transformacni matice!
 * Upravte tak, aby se model vykreslil interpolovane dle parametru n
 * (cela cast n udava klicovy snimek, desetinna cast n parametr interpolace
 * mezi snimkem n a n + 1)
 * i - index trojuhelniku
 * n - index klicoveho snimku (float pro pozdejsi interpolaci mezi snimky)
 */

void studrenProjectTriangle(S_Renderer *pRenderer, S_Model *pModel, int i, float n)

{
	S_Coords    aa, bb, cc;             /* souradnice vrcholu po transformaci */
	S_Coords    naa, nbb, ncc;          /* normaly ve vrcholech po transformaci */
	S_Coords    nn;                     /* normala trojuhelniku po transformaci */
	int         u1, v1, u2, v2, u3, v3; /* souradnice vrcholu po projekci do roviny obrazovky */
	S_Triangle  * triangle;
	int         vertexOffset, normalOffset; /* offset pro vrcholy a normalove vektory trojuhelniku */
	int         i0, i1, i2, in;             /* indexy vrcholu a normaly pro i-ty trojuhelnik n-teho snimku */

	/* n+1 snimok*/
	int         vertexOffsetNext, normalOffsetNext; /* offset pro vrcholy a normalove vektory trojuhelniku */
	int         i0Next, i1Next, i2Next, inNext;     /* indexy vrcholu a normaly pro i-ty trojuhelnik n-teho snimku */

	/* pre interpolaciu */
	S_Coords    aIP, bIP, cIP;             /* souradnice vrcholu po interpolaci */
	S_Coords    naIP, nbIP, ncIP;          /* normaly ve vrcholech po transformaci */
	S_Coords    nIP;                     /* normala trojuhelniku po transformaci */

	/* Stvrta homogenna suradnica */
	double h1, h2, h3;

	IZG_ASSERT(pRenderer && pModel && i >= 0 && i < trivecSize(pModel->triangles) && n >= 0);

	/* z modelu si vytahneme i-ty trojuhelnik */
	triangle = trivecGetPtr(pModel->triangles, i);

	/* ziskame offset pro vrcholy n-teho snimku */
	vertexOffset = (((int)n) % pModel->frames) * pModel->verticesPerFrame;
	/* ziskame offset pro vrcholy n+1-teho snimku */
	vertexOffsetNext = (((int)n+1) % pModel->frames) * pModel->verticesPerFrame;

	/* ziskame offset pro normaly trojuhelniku n-teho snimku */
	normalOffset = (((int)n) % pModel->frames) * pModel->triangles->size;
	/* ziskame offset pro normaly trojuhelniku n+1-teho snimku */
	normalOffsetNext = (((int)n+1) % pModel->frames) * pModel->triangles->size;

	/* indexy vrcholu pro i-ty trojuhelnik n-teho snimku - pricteni offsetu */
	i0 = triangle->v[0] + vertexOffset;
	i1 = triangle->v[1] + vertexOffset;
	i2 = triangle->v[2] + vertexOffset;

	/* indexy vrcholu pro i-ty trojuhelnik n+1-teho snimku - pricteni offsetu */
	i0Next = triangle->v[0] + vertexOffsetNext;
	i1Next = triangle->v[1] + vertexOffsetNext;
	i2Next = triangle->v[2] + vertexOffsetNext;

	/* index normaloveho vektoru pro i-ty trojuhelnik n-teho snimku - pricteni offsetu */
	in = triangle->n + normalOffset;
	/* index normaloveho vektoru pro i-ty trojuhelnik n+1-teho snimku - pricteni offsetu */
	inNext = triangle->n + normalOffsetNext;

	/* Interpolace */
	double fracPart = n - (int)n;

	aIP.x = cvecGetPtr(pModel->vertices, i0)->x * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i0Next)->x * (fracPart);
	aIP.y = cvecGetPtr(pModel->vertices, i0)->y * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i0Next)->y * (fracPart);
	aIP.z = cvecGetPtr(pModel->vertices, i0)->z * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i0Next)->z * (fracPart);

	bIP.x = cvecGetPtr(pModel->vertices, i1)->x * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i1Next)->x * (fracPart);
	bIP.y = cvecGetPtr(pModel->vertices, i1)->y * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i1Next)->y * (fracPart);
	bIP.z = cvecGetPtr(pModel->vertices, i1)->z * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i1Next)->z * (fracPart);
	
	cIP.x = cvecGetPtr(pModel->vertices, i2)->x * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i2Next)->x * (fracPart);
	cIP.y = cvecGetPtr(pModel->vertices, i2)->y * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i2Next)->y * (fracPart);
	cIP.z = cvecGetPtr(pModel->vertices, i2)->z * (1.0 - fracPart) + cvecGetPtr(pModel->vertices, i2Next)->z * (fracPart);

	naIP.x = cvecGetPtr(pModel->normals, i0)->x * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i0Next)->x * (fracPart);
	naIP.y = cvecGetPtr(pModel->normals, i0)->y * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i0Next)->y * (fracPart);
	naIP.z = cvecGetPtr(pModel->normals, i0)->z * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i0Next)->z * (fracPart);

	nbIP.x = cvecGetPtr(pModel->normals, i1)->x * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i1Next)->x * (fracPart);
	nbIP.y = cvecGetPtr(pModel->normals, i1)->y * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i1Next)->y * (fracPart);
	nbIP.z = cvecGetPtr(pModel->normals, i1)->z * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i1Next)->z * (fracPart);

	ncIP.x = cvecGetPtr(pModel->normals, i2)->x * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i2Next)->x * (fracPart);
	ncIP.y = cvecGetPtr(pModel->normals, i2)->y * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i2Next)->y * (fracPart);
	ncIP.z = cvecGetPtr(pModel->normals, i2)->z * (1.0 - fracPart) + cvecGetPtr(pModel->normals, i2Next)->z * (fracPart);

	nIP.x = cvecGetPtr(pModel->trinormals, in)->x * (1.0 - fracPart) + cvecGetPtr(pModel->trinormals, inNext)->x * (fracPart);
	nIP.y = cvecGetPtr(pModel->trinormals, in)->y * (1.0 - fracPart) + cvecGetPtr(pModel->trinormals, inNext)->y * (fracPart);
	nIP.z = cvecGetPtr(pModel->trinormals, in)->z * (1.0 - fracPart) + cvecGetPtr(pModel->trinormals, inNext)->z * (fracPart);

	/* transformace vrcholu matici model */
	trTransformVertex(&aa, &aIP);
	trTransformVertex(&bb, &bIP);
	trTransformVertex(&cc, &cIP);

	/* promitneme vrcholy trojuhelniku na obrazovku */
	h1 = trProjectVertex(&u1, &v1, &aa);
	h2 = trProjectVertex(&u2, &v2, &bb);
	h3 = trProjectVertex(&u3, &v3, &cc);

	/* pro osvetlovaci model transformujeme take normaly ve vrcholech */
	trTransformVector(&naa, &naIP);
	trTransformVector(&nbb, &nbIP);
	trTransformVector(&ncc, &ncIP);

	/* normalizace normal */
	coordsNormalize(&naa);
	coordsNormalize(&nbb);
	coordsNormalize(&ncc);

	/* transformace normaly trojuhelniku matici model */
	trTransformVector(&nn, &nIP);

	/* normalizace normaly */
	coordsNormalize(&nn);

	/* je troj. privraceny ke kamere, tudiz viditelny? */
	if (!renCalcVisibility(pRenderer, &aa, &nn))
	{
		/* odvracene troj. vubec nekreslime */
		return;
	}

	/* rasterizace trojuhelniku */
	studrenDrawTriangle(pRenderer,
		&aa, &bb, &cc,
		&naa, &nbb, &ncc,
		u1, v1, u2, v2, u3, v3,
		triangle->t,
		h1, h2, h3
	);
}

/******************************************************************************
* Vraci hodnotu v aktualne nastavene texture na zadanych
* texturovacich souradnicich u, v
* Pro urceni hodnoty pouziva bilinearni interpolaci
* Pro otestovani vraci ve vychozim stavu barevnou sachovnici dle uv souradnic
* u, v - texturovaci souradnice v intervalu 0..1, ktery odpovida sirce/vysce textury
*/

S_RGBA studrenTextureValue( S_StudentRenderer * pRenderer, double u, double v )
{
	if (isnan(u) || isnan(v)) return makeColor(0, 0, 0);

	int SIZE = pRenderer->w;
	u = (u*(pRenderer->w-1));
	v = (v*(pRenderer->h-1));

	int x = (int)u;
	int xx = x + 1;
	int y = (int)v;
	int yy = y + 1;

	int xSize = x*SIZE;
	int xxSize = xx*SIZE;
	int i11 = xSize + y;
	int i12 = xSize + yy;
	int i21 = xxSize + y;
	int i22 = xxSize + yy;
	
	double x2x = xx - u;
	double y2y = yy - v;
	double xx1 = u - x;
	double yy1 = v - y;

	double x2xy2y = x2x * y2y;
	double xx1y2y = xx1 * y2y;
	double x2xyy1 = x2x * yy1;
	double xx1yy1 = xx1 * yy1;

	unsigned char r = ROUND2BYTE(
					  pRenderer->color[i11].red * x2xy2y
					+ pRenderer->color[i21].red * xx1y2y 
					+ pRenderer->color[i12].red * x2xyy1 
					+ pRenderer->color[i22].red * xx1yy1);
	unsigned char g = ROUND2BYTE(
					  pRenderer->color[i11].green * x2xy2y
					+ pRenderer->color[i21].green * xx1y2y
					+ pRenderer->color[i12].green * x2xyy1
					+ pRenderer->color[i22].green * xx1yy1);
	unsigned char b = ROUND2BYTE(
		              pRenderer->color[i11].blue * x2xy2y
					+ pRenderer->color[i21].blue * xx1y2y
					+ pRenderer->color[i12].blue * x2xyy1
					+ pRenderer->color[i22].blue * xx1yy1);

	return makeColor(r, g, b);
}


/******************************************************************************
 ******************************************************************************
 * Funkce pro vyrenderovani sceny, tj. vykresleni modelu
 * Upravte tak, aby se model vykreslil animovane
 * (volani renderModel s aktualizovanym parametrem n)
 */

void renderStudentScene(S_Renderer *pRenderer, S_Model *pModel)

{
	S_Material MAT_WHITE_AMBIENT = { 1.0, 1.0, 1.0, 1.0 };
	S_Material MAT_WHITE_DIFFUSE = { 1.0, 1, 1.0, 1.0 };
	S_Material MAT_WHITE_SPECULAR = { 1.0, 1.0, 1.0, 1.0 };

	/* test existence frame bufferu a modelu */
	IZG_ASSERT(pModel && pRenderer);

	/* nastavit projekcni matici */
	trProjectionPerspective(pRenderer->camera_dist, pRenderer->frame_w, pRenderer->frame_h);

	/* vycistit model matici */
	trLoadIdentity();

	/* nejprve nastavime posuv cele sceny od/ke kamere */
	trTranslate(0.0, 0.0, pRenderer->scene_move_z);

	/* nejprve nastavime posuv cele sceny v rovine XY */
	trTranslate(pRenderer->scene_move_x, pRenderer->scene_move_y, 0.0);

	/* natoceni cele sceny - jen ve dvou smerech - mys je jen 2D... :( */
	trRotateX(pRenderer->scene_rot_x);
	trRotateY(pRenderer->scene_rot_y);

	/* nastavime material */
	renMatAmbient(pRenderer, &MAT_WHITE_AMBIENT);
	renMatDiffuse(pRenderer, &MAT_WHITE_DIFFUSE);
	renMatSpecular(pRenderer, &MAT_WHITE_SPECULAR);

	/* a vykreslime nas model (ve vychozim stavu kreslime pouze snimek 0) */

	renderModel(pRenderer, pModel, TIME);
}

/* Callback funkce volana pri tiknuti casovace
 * ticks - pocet milisekund od inicializace */
void onTimer( int ticks )
{
    /* uprava parametru pouzivaneho pro vyber klicoveho snimku
     * a pro interpolaci mezi snimky */
	TIME = ticks * 0.01f;
}

/*****************************************************************************
 *****************************************************************************/
