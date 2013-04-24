#include "tab-container.h"

#include "util/font.h"
#include "util/debug.h"

namespace Gui{

TabItem::TabItem():
active(false){
}

TabItem::TabItem(const std::string & name):
active(false),
name(name){
}

TabItem::~TabItem(){
}

DummyTab::DummyTab(const std::string & name):
TabItem(name){
}

DummyTab::~DummyTab(){
}

void DummyTab::act(){
}

void DummyTab::draw(const Font&, const Graphics::Bitmap & work){
    work.fill(Graphics::makeColor(220, 220, 220));
}

TabContainer::TabContainer(){
}

TabContainer::TabContainer(const TabContainer & copy){
}

TabContainer::~TabContainer(){
}

TabContainer & TabContainer::operator=(const TabContainer & copy){
    return *this;
}

void TabContainer::act(const Font & font){
}

void TabContainer::render(const Graphics::Bitmap &){
}

static void drawBox(int radius, int x1, int y1, int x2, int y2, Gui::ColorInfo colors, const Graphics::Bitmap & work){
    // rounded body?
    if (radius > 0){
        if (colors.bodyAlpha < 255){
            Graphics::Bitmap::transBlender(0,0,0,colors.bodyAlpha);
            work.translucent().roundRectFill(radius, x1, y1, x2, y2, colors.body);
            Graphics::Bitmap::transBlender(0,0,0,colors.borderAlpha);
            work.translucent().roundRect(radius, x1, y1, x2-1, y2-1, colors.border);
        } else {
            work.roundRectFill(radius, x1, y1, x2, y2, colors.body);
            work.roundRect(radius, x1, y1, x2-1, y2-1, colors.border);
        }
    } else {
        if (colors.bodyAlpha < 255){
            Graphics::Bitmap::transBlender(0,0,0,colors.bodyAlpha);
            work.translucent().rectangleFill(x1, y1, x2, y2, colors.body );
            Graphics::Bitmap::transBlender(0,0,0,colors.borderAlpha);
            work.translucent().vLine(y1,x1,y2-1,colors.border);
            work.translucent().hLine(x1,y2-1,x2,colors.border);
            work.translucent().vLine(y1,x2-1,y2-1,colors.border);
        } else {
            work.rectangleFill(x1, y1, x2, y2, colors.body );
            work.vLine(y1,x1,y2-1,colors.border);
            work.hLine(x1,y2-1,x2,colors.border);
            work.vLine(y1,x2-1,y2-1,colors.border);
        }
    }
}

void TabContainer::draw(const Font & font, const Graphics::Bitmap & work){
    const int tabHeight = font.getHeight();
    const int height = location.getHeight() - tabHeight+1;
    
    // Draw tabs
    drawTabs(font, Graphics::Bitmap(work, location.getX(), location.getY(), location.getWidth(), tabHeight+1));
    
    Graphics::Bitmap area(work, location.getX(), location.getY() + tabHeight+1, location.getWidth(), height);
    drawBox(transforms.getRadius(), 0, -(tabHeight+1), location.getWidth(), height, colors, area);
}

void TabContainer::add(Util::ReferenceCount<TabItem> tab){
    tabs.push_back(tab);
    if (tabs.size() == 1){
        tab->toggleActive();
    }
}

void TabContainer::drawTabs(const Font & font, const Graphics::Bitmap & work){
    if (tabs.empty()){
        drawBox(transforms.getRadius(), 0, 0, work.getWidth(), work.getHeight()*2, colors, work);
        font.printf((work.getWidth()/2) - (font.textLength("Empty")/2), 0, Graphics::makeColor(255,255,255), work, "Empty", 0);
        return;
    }
    const int width = work.getWidth() / tabs.size();
    const int inactiveY = work.getHeight() * .25;
    int currentX = 0;
    for (std::vector< Util::ReferenceCount<TabItem> >::iterator i = tabs.begin(); i != tabs.end(); ++i){
        Util::ReferenceCount<TabItem> tab = *i;
        if (tab->isActive()){
            drawBox(transforms.getRadius(), currentX, 0, width, work.getHeight()*2, colors, work);
            font.printf(currentX + (width/2) - (font.textLength(tab->getName().c_str())/2), 0, Graphics::makeColor(255,255,255), work, tab->getName(), 0);
        } else {
            drawBox(transforms.getRadius(), currentX, inactiveY, width, work.getHeight()*2, colors, work);
            font.printf(currentX + (width/2) - (font.textLength(tab->getName().c_str())/2), inactiveY, Graphics::makeColor(255,255,255), work, tab->getName(), 0);
            if (colors.bodyAlpha < 255){
                Graphics::Bitmap::transBlender(0,0,0,colors.borderAlpha);
                work.translucent().hLine(currentX,work.getHeight()-1,currentX + width,colors.border);
            } else {
                work.hLine(currentX,work.getHeight()-1,currentX + width,colors.border);
            }
        }
        currentX += width;
    }
}

}
