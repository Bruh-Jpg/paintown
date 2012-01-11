#include "funcs.h"
#include "bitmap.h"
#include "trans-bitmap.h"
#include "stretch-bitmap.h"
#include "lit_bitmap.h"
#include <string>
#include <stdio.h>
#include <math.h>

namespace Graphics{

static Bitmap * Screen = NULL;
/* bitmaps that should always be resized to the dimensions of the screen */
static std::vector<Bitmap*> needResize;

Util::Parameter<Bitmap*> screenParameter;

/* implementation independant definitions can go here */

Bitmap * Bitmap::temporary_bitmap = NULL;
Bitmap * Bitmap::temporary_bitmap2 = NULL;

int SCALE_X = 0;
int SCALE_Y = 0;
const int Bitmap::MODE_TRANS = 0;
const int Bitmap::MODE_SOLID = 1;
	
const int SPRITE_NO_FLIP = 0;
const int SPRITE_V_FLIP = 1;
const int SPRITE_H_FLIP = 2;
	
const int SPRITE_NORMAL = 1;
const int SPRITE_LIT = 2;
const int SPRITE_TRANS = 3;

static inline int max(int a, int b){
    return a > b ? a : b;
}
	
void initializeExtraStuff();

QualityFilter qualityFilterName(const std::string & type){
    if (type == "xbr"){
        return XbrFilter;
    }
    if (type == "hqx"){
        return HqxFilter;
    }
    return NoFilter;
}

Bitmap::~Bitmap(){
    if (mustResize){
        for (std::vector<Bitmap*>::iterator it = needResize.begin(); it != needResize.end(); it++){
            Bitmap * who = *it;
            if (who == this){
                needResize.erase(it);
                break;
            }
        }
    }
}

static Bitmap makeTemporaryBitmap(Bitmap *& temporary, int w, int h){
    if (temporary == NULL){
        temporary = new Bitmap(w, h);
    } else if (temporary->getWidth() < w || temporary->getHeight() < h){
        int mw = max(temporary->getWidth(), w);
        int mh = max(temporary->getHeight(), h);
        // printf("Create temporary bitmap %d %d\n", mw, mh);
        delete temporary;
        temporary = new Bitmap(mw, mh);
    }
    if (temporary == NULL){
        printf("*bug* temporary bitmap is null\n");
    }
    return Bitmap(*temporary, 0, 0, w, h);
}

Bitmap Bitmap::temporaryBitmap(int w, int h){
    return makeTemporaryBitmap(temporary_bitmap, w, h);
}

Bitmap Bitmap::temporaryBitmap2(int w, int h){
    return makeTemporaryBitmap(temporary_bitmap2, w, h);
}
        
void Bitmap::cleanupTemporaryBitmaps(){
    if (temporary_bitmap != NULL){
        delete temporary_bitmap;
        temporary_bitmap = NULL;
    }

    if (temporary_bitmap2 != NULL){
        delete temporary_bitmap2;
        temporary_bitmap2 = NULL;
    }
}

Bitmap & Bitmap::operator=(const Bitmap & copy){
    path = copy.getPath();
    setData(copy.getData());
    return *this;
}

double Bitmap::getScale(){
    /* the game is pretty much hard coded to run at 320 scaled upto 640
     * and then scaled to whatever the user wants, but as long as
     * 320 and 640 remain this number will be 2.
     * maybe calculate this at some point
     */
    return 2;
    /*
    if (Scaler != NULL && Buffer != NULL){
        double x1 = Scaler->getWidth();
        double x2 = Buffer->getWidth();
        return x2 / x1;
    }
    return 1;
    */
}

/* taken from the color addon from allegro 4.9 */
static void al_color_cmyk_to_rgb(float cyan, float magenta, float yellow, float key, float *red, float *green, float *blue){
    float max = 1 - key;
    *red = max - cyan * max;
    *green = max - magenta * max;
    *blue = max - yellow * max;
}

void Bitmap::cymkToRGB(int c, int y, int m, int k, int * r, int * g, int * b){
    float fc = (float)c / 255.0;
    float fy = (float)y / 255.0;
    float fm = (float)m / 255.0;
    float fk = (float)k / 255.0;

    float fr, fg, fb;
    al_color_cmyk_to_rgb(fc, fm, fy, fk, &fr, &fg, &fb);
    *r = (int)(fr * 255.0);
    *g = (int)(fg * 255.0);
    *b = (int)(fb * 255.0);
}
        
void Bitmap::updateOnResize(){
    if (!mustResize){
        mustResize = true;
        needResize.push_back(this);
    }
}
        
void Bitmap::updateSize(const int width, const int height){
    if (getWidth() == width && getHeight() == height){
        return;
    }

    Bitmap created(width, height);
    *this = created;
}

/* resize the internal bitmap. not guaranteed to destroy the internal bitmap */
void Bitmap::resize(const int width, const int height){

    /* if internal bitmap is already the proper size, do nothing */
    if (getWidth() == width && getHeight() == height){
        return;
    }

    Bitmap created(width, height);
    Stretch(created);
    *this = created;
}


/* decrement bitmap reference counter and free memory if counter hits 0 */
#if 0
void Bitmap::releaseInternalBitmap(){
    const int MAGIC_DEBUG = 0xa5a5a5;
    if (own != NULL){
        if (*own == MAGIC_DEBUG){
            printf("[bitmap] Trying to delete an already deleted reference counter %p\n", own);
        }

        (*own) -= 1;
        if ( *own == 0 ){
            *own = MAGIC_DEBUG;
            delete own;
            destroyPrivateData();
            own = NULL;
        }
    }
}
#endif

void Bitmap::BlitToScreen() const {
    // this->Blit( *Bitmap::Screen );
    this->BlitToScreen(0, 0);
}

void Bitmap::load( const std::string & str ){
    // releaseInternalBitmap();
    internalLoadFile( str.c_str() );
}
        
Bitmap Bitmap::scaleBy(const double widthRatio, const double heightRatio) const {
    return scaleTo(getWidth() * widthRatio, getHeight() * heightRatio);
}

void Bitmap::border( int min, int max, Color color ) const {
    int w = getWidth();
    int h = getHeight();
    for (int i = min; i < max; i++){
        rectangle(i, i, w - 1 - i, h - 1 - i, color);
    }
}

void Bitmap::drawHFlip(const int x, const int y, const int startWidth, const int startHeight, const int width, const int height, const Bitmap & where) const {
    drawHFlip(x, y, startWidth, startHeight, width, height, NULL, where);
}

void Bitmap::drawHFlip(const int x, const int y, const int startWidth, const int startHeight, const int width, const int height, Filter * filter, const Bitmap & where) const {
    Bitmap sub(*this, getWidth() - width, getHeight() - height, getWidth() - startWidth, getHeight() - startHeight);
    sub.drawHFlip(x + startWidth, y + startHeight, filter, where);
}

void Bitmap::drawRotateCenter(const int x, const int y, const int angle, const Bitmap & where){
    drawRotate(x - getWidth() / 2, y - getHeight() / 2, angle, where);
}

void Bitmap::drawCenter(const int x, const int y, const Bitmap & where) const {
    draw(x - getWidth() / 2, y - getHeight() / 2, where);
}
	
void Bitmap::drawStretched(const Bitmap & who) const {
    drawStretched(0, 0, who.getWidth(), who.getHeight(), who);
}

void Bitmap::draw(const int x, const int y, const int startWidth, const int startHeight, const int width, const int height, const Bitmap & where) const {
    draw(x, y, startWidth, startHeight, width, height, NULL, where);
    /*
    Bitmap sub(*this, startWidth, startHeight, width, height);
    sub.draw(x + startWidth, y + startHeight, where);
    */
}

void Bitmap::draw(const int x, const int y, const int startWidth, const int startHeight, const int width, const int height, Filter * filter, const Bitmap & where) const {
    Bitmap sub(*this, startWidth, startHeight, width, height);
    sub.draw(x + startWidth, y + startHeight, filter, where);
}

void Bitmap::horizontalLine( const int x1, const int y, const int x2, const Graphics::Color color ) const{
    this->hLine(x1, y, x2, color);
}
        
void Bitmap::equilateralTriangle(int x, int y, int angle, int size, Color color) const {
    double radians = Util::radians(angle);
    int x1 = x + size / 2 * cos(radians + 2 * Util::pi / 3);
    int y1 = y + size / 2 * sin(radians + 2 * Util::pi / 3);
    int x2 = x + size / 2 * cos(radians - 2 * Util::pi / 3);
    int y2 = y + size / 2 * sin(radians - 2 * Util::pi / 3);
    int x3 = x + size / 2 * cos(radians);
    int y3 = y + size / 2 * sin(radians);
    triangle(x1, y1, x2, y2, x3, y3, color);
}

Bitmap Bitmap::greyScale(){
    Bitmap grey(getWidth(), getHeight());

    for (int x = 0; x < getWidth(); x++){
        for (int y = 0; y < getHeight(); y++){
	    Color pixel = getPixel(x, y);
            int val = (int)((0.299*getRed(pixel) + 0.587*getGreen(pixel) + 0.114*getBlue(pixel) + 0.5) + 16);
            if (val > 255){
                val = 255;
            }
            grey.putPixel(x, y, makeColor(val, val, val));
        }
    }

    return grey;
}
        
bool Bitmap::inRange(int x, int y) const {
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    getClipRect(x1, y1, x2, y2);
    return (x >= x1 && x <= x2 &&
            y >= y1 && y <= y2);
}

void Bitmap::drawMask( const int _x, const int _y, const Bitmap & where ){
    Color mask = MaskColor();
    for (int x = 0; x < getWidth(); x++){
        for (int y = 0; y < getHeight(); y++){
            if (getPixel(x,y) == mask){
                where.putPixel(x+_x, y+_y, mask);
            }
        }
    }
}
        
void Bitmap::set8BitMaskColor(const Color & color){
    bit8MaskColor = color;
}
        
Color Bitmap::get8BitMaskColor(){
    return bit8MaskColor;
}

void Bitmap::setFakeGraphicsMode(int width, int height){
    initializeExtraStuff();
    Screen = new Bitmap(width, height);
}

void Bitmap::Blit( const std::string & xpath ) const {
    Bitmap duh(xpath);
    duh.Blit(*this);
}

void Bitmap::Blit(const Bitmap & where) const {
    this->Blit(0, 0, where);
}

void Bitmap::Blit(const int x, const int y, const Bitmap & where) const {
    Blit(0, 0, x, y, where);
}

void Bitmap::Blit(const int mx, const int my, const int wx, const int wy, const Bitmap & where) const {
    Blit(mx, my, getWidth(), getHeight(), wx, wy, where);
}

void Bitmap::BlitFromScreen(const int x, const int y) const {
    Screen->Blit(x, y, getWidth(), getHeight(), 0, 0, *this);
}

//! min (borrowed from allegro)
static inline int Min(int x, int y){ return (((x) < (y)) ? (x) : (y)); }

//! max (borrowed from allegro)
static inline int Max(int x, int y){ return (((x) > (y)) ? (x) : (y)); }

//! mid (borrowed from allegro)
static inline int Mid(int x,int y,int z){ return (Max((x), Min((y), (z)))); }

void Bitmap::roundRect(int radius, int x1, int y1, int x2, int y2, Color color) const {
    const int width = x2 - x1;
    const int height = y2 - y1;
    radius = Mid(0, radius, Min((x1+width - x1)/2, (y1+height - y1)/2));
    line(x1+radius, y1, x1+width-radius, y1, color);
    line(x1+radius, y1+height, x1+width-radius,y1+height, color);
    line(x1, y1+radius,x1, y1+height-radius, color);
    line(x1+width, y1+radius,x1+width, y1+height-radius, color);

    double quarterTurn = Util::pi / 2;
    double quadrant1 = 0;
    /* signs are flipped because the coordinate system is reflected over the y-axis */
    double quadrant2 = -Util::pi / 2;
    double quadrant3 = Util::pi;
    double quadrant4 = -3 * Util::pi / 2; 

    /* upper right. draw from 90 to 0 */
    arc(x1+radius + (width - radius *2), y1 + radius, quadrant1, quadrant1 + quarterTurn, radius, color);
    /* upper left. draw from 180 to 270 */
    arc(x1 + radius, y1 + radius, quadrant2, quadrant2 + quarterTurn, radius, color);
    /* lower left. draw from 180 to 270 */
    arc(x1 + radius, y1 + height - radius, quadrant3, quadrant3 + quarterTurn, radius, color);
    /* lower right. draw from 0 to 270 */
    arc(x1+width-radius, y1+height-radius, quadrant4, quadrant4 + quarterTurn, radius, color);
}

void Bitmap::roundRectFill(int radius, int x1, int y1, int x2, int y2, Graphics::Color color) const {
    const int width = x2 - x1;
    const int height = y2 - y1;
    radius = Mid(0, radius, Min((x1+width - x1)/2, (y1+height - y1)/2));

    double quarterTurn = Util::pi / 2;
    double quadrant1 = 0;
    /* signs are flipped because the coordinate system is reflected over the y-axis */
    double quadrant2 = -Util::pi / 2;
    double quadrant3 = Util::pi;
    double quadrant4 = -3 * Util::pi / 2; 

    /* upper right. draw from 90 to 0 */
    arcFilled(x1+radius + (width - radius *2), y1 + radius, quadrant1, quadrant1 + quarterTurn, radius, color);
    /* upper left. draw from 180 to 270 */
    arcFilled(x1 + radius, y1 + radius, quadrant2, quadrant2 + quarterTurn, radius, color);
    /* lower left. draw from 180 to 270 */
    arcFilled(x1 + radius, y1 + height - radius, quadrant3, quadrant3 + quarterTurn, radius, color);
    /* lower right. draw from 0 to 270 */
    arcFilled(x1+width-radius, y1+height-radius, quadrant4, quadrant4 + quarterTurn, radius, color);

    rectangleFill(x1+radius + 1, y1, x2-radius - 1, y1+radius - 1, color);
    rectangleFill(x1, y1+radius, x2, y2-radius, color);
    rectangleFill(x1+radius + 1, y2-radius + 1, x2-radius - 1, y2, color);
}

int Bitmap::getScreenWidth(){
    if (Screen != 0){
        return Screen->getWidth();
    }
    return 0;
}

int Bitmap::getScreenHeight(){
    if (Screen != 0){
        return Screen->getHeight();
    }
    return 0;
}

void Bitmap::clear() const {
    fill(makeColor(0, 0, 0));
}

void Bitmap::Stretch( const Bitmap & where ) const {
    if (getWidth() == where.getWidth() && getHeight() == where.getHeight()){
        Blit(where);
    } else {
        Stretch(where, 0, 0, getWidth(), getHeight(), 0, 0, where.getWidth(), where.getHeight());
    }
}

void Bitmap::StretchHqx(const Bitmap & where) const {
    if (getWidth() == where.getWidth() && getHeight() == where.getHeight()){
        Blit(where);
    } else {
        StretchHqx(where, 0, 0, getWidth(), getHeight(), 0, 0, where.getWidth(), where.getHeight());
    }
}

void Bitmap::StretchXbr(const Bitmap & where) const {
    if (getWidth() == where.getWidth() && getHeight() == where.getHeight()){
        Blit(where);
    } else {
        StretchXbr(where, 0, 0, getWidth(), getHeight(), 0, 0, where.getWidth(), where.getHeight());
    }
}

Color darken(Color color, double factor ){
    int r = (int)((double)getRed(color) / factor);
    int g = (int)((double)getGreen(color) / factor);
    int b = (int)((double)getBlue(color) / factor);

    return makeColor(r, g, b);
}

LitBitmap::LitBitmap(const Bitmap & b):
Bitmap(b){
    int x1, y1, x2, y2;
    b.getClipRect(x1, y1, x2, y2);
    setClipRect(x1, y1, x2, y2);
}

LitBitmap::LitBitmap():
Bitmap(){
}
	
LitBitmap::~LitBitmap(){
}

LitBitmap Bitmap::lit() const {
    return LitBitmap(*this);
}

TranslucentBitmap Bitmap::translucent() const {
    return TranslucentBitmap(*this);
}
        
TranslucentBitmap Bitmap::translucent(int red, int green, int blue, int alpha) const {
    transBlender(red, green, blue, alpha);
    return TranslucentBitmap(*this);
}

TranslucentBitmap::TranslucentBitmap(const Bitmap & b):
Bitmap(b){
    int x1, y1, x2, y2;
    b.getClipRect(x1, y1, x2, y2);
    setClipRect(x1, y1, x2, y2);
}

TranslucentBitmap::TranslucentBitmap():
Bitmap(){
}

TranslucentBitmap::~TranslucentBitmap(){
}

void TranslucentBitmap::fill(Color color) const {
    Bitmap::applyTrans(color);
}

int StretchedBitmap::getWidth() const {
    return width;
}

int StretchedBitmap::getHeight() const {
    return height;
}
    
double StretchedBitmap::getScaleWidth() const {
    return (double) where.getWidth() / (double) getWidth();
}

double StretchedBitmap::getScaleHeight() const {
    return (double) where.getHeight() / (double) getHeight();
}

void blend_palette(Color * pal, int mp, Color startColor, Color endColor){
    /*
    ASSERT(pal);
    ASSERT(mp != 0);
    */

    int sc_r = Graphics::getRed(startColor);
    int sc_g = Graphics::getGreen(startColor);
    int sc_b = Graphics::getBlue(startColor);

    int ec_r = Graphics::getRed(endColor);
    int ec_g = Graphics::getGreen(endColor);
    int ec_b = Graphics::getBlue(endColor);

    for ( int q = 0; q < mp; q++ ) {
        float j = (float)( q + 1 ) / (float)( mp );
        int f_r = (int)( 0.5 + (float)( sc_r ) + (float)( ec_r-sc_r ) * j );
        int f_g = (int)( 0.5 + (float)( sc_g ) + (float)( ec_g-sc_g ) * j );
        int f_b = (int)( 0.5 + (float)( sc_b ) + (float)( ec_b-sc_b ) * j );
        pal[q] = Graphics::makeColor( f_r, f_g, f_b );
    }
}

}

#ifdef USE_ALLEGRO
#include "allegro/bitmap.cpp"
#endif
#ifdef USE_SDL
#include "sdl/bitmap.cpp"
#endif
#ifdef USE_ALLEGRO5
#include "allegro5/bitmap.cpp"
#endif

