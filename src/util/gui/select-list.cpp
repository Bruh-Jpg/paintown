#include "select-list.h"

#include "util/bitmap.h"
#include "util/font.h"
#include "util/debug.h"

#include <math.h>

using namespace Gui;

SelectItem::SelectItem(){
}
SelectItem::~SelectItem(){
}

SelectListInterface::SelectListInterface():
accessEmpty(true),
allowWrap(true){
}

SelectListInterface::~SelectListInterface(){
}

SimpleSelect::SimpleSelect():
layout(Horizontal),
viewable(3),
currentTop(0),
scrollOffset(0),
cellWidth(100),
cellHeight(100),
cellSpacingX(0),
cellSpacingY(0),
cellMarginX(0),
cellMarginY(0){
}
SimpleSelect::~SimpleSelect(){
}
void SimpleSelect::act(){
    /* Check if viewable is larger than the current items */
    if (viewable >= items.size()){
        viewable = items.size()-1;
    }
}
void SimpleSelect::render(const Graphics::Bitmap & work, const Font & font) const{
    int x = cellMarginX;
    int y = cellMarginY;
    const int stop = currentTop + viewable;
    int count = currentTop;
    for (std::vector<Util::ReferenceCount<SelectItem> >::const_iterator i = items.begin() + currentTop; i != items.end() && count != stop; ++i, ++count){
        const Util::ReferenceCount<SelectItem> item = *i;
        item->draw(x, y, cellWidth, cellHeight, work, font);
        x+=cellSpacingX + (layout == Horizontal ? cellWidth + cellMarginX : 0);
        y+=cellSpacingY + (layout == Vertical ? cellHeight + cellMarginY : 0);
    }
}
void SimpleSelect::addItem(const Util::ReferenceCount<SelectItem> & item){
    items.push_back(item);
}
void SimpleSelect::addItems(const std::vector<Util::ReferenceCount<SelectItem> > & itemList){
    items.insert(items.begin(), itemList.begin(), itemList.end());
}
const std::vector<Util::ReferenceCount<SelectItem> > & SimpleSelect::getItems() const{
    return items;
}
void SimpleSelect::clearItems(){
    items.clear();
}
void SimpleSelect::setCellDimensions(int width, int height){
    cellWidth = width;
    cellHeight = height;
}
void SimpleSelect::setCellSpacing(int x, int y){
    cellSpacingX = x;
    cellSpacingY = y;
}
void SimpleSelect::setCellMargins(int x, int y){
    cellMarginX = x;
    cellMarginY = y;
}
void SimpleSelect::setCursors(int total){
    cursors.resize(total);
}
int SimpleSelect::totalCursors() const{
    return cursors.size();
}
void SimpleSelect::setCurrentIndex(int cursor, unsigned int location){
    if (checkCursor(cursor) && location >= items.size()){
        return;
    }
    cursors[cursor] = location;
}
unsigned int SimpleSelect::getCurrentIndex(int cursor) const{
    if (checkCursor(cursor)){
        return 0;
    }
    return cursors[cursor];
}
/* NOTE This doesn't account for other cursors and viewable areas */
bool SimpleSelect::up(int cursor){
    if (checkCursor(cursor)){
        return 0;
    }
    if (cursors[cursor] > 0){
        cursors[cursor]--;
        calculateLeft(cursor);
        return true;
    } else if (allowWrap){
        cursors[cursor] = items.size()-1;
        currentTop = cursors[cursor] - viewable+1;
        return true;
    }
    return false;
}
bool SimpleSelect::down(int cursor){
    if (checkCursor(cursor)){
        return 0;
    }
    if (cursors[cursor] < items.size()-1){
        cursors[cursor]++;
        calculateRight(cursor);
        return true;
    } else if (allowWrap){
        cursors[cursor] = currentTop = 0;
        return true;
    }
    return false;
}
bool SimpleSelect::left(int cursor){
    if (checkCursor(cursor)){
        return 0;
    }
    if (cursors[cursor] > 0){
        cursors[cursor]--;
        calculateLeft(cursor);
        return true;
    } else if (allowWrap){
        cursors[cursor] = items.size()-1;
        currentTop = cursors[cursor] - viewable + 1;
        return true;
    }
    return false;
}

bool SimpleSelect::right(int cursor){
    if (checkCursor(cursor)){
        return 0;
    }
    if (cursors[cursor] < items.size()-1){
        cursors[cursor]++;
        calculateRight(cursor);
        return true;
    } else if (allowWrap){
        cursors[cursor] = currentTop = 0;
        return true;
    }
    return false;
}

bool SimpleSelect::checkCursor(int cursor) const {
    return ((unsigned int)cursor >= cursors.size());
}

void SimpleSelect::calculateLeft(int cursor){
    if (currentTop == 0){
        //currentTop = cursors[cursor];
    } else if (cursors[cursor] < currentTop + scrollOffset){
        currentTop = cursors[cursor] - scrollOffset;
    }
}

void SimpleSelect::calculateRight(int cursor){
    const unsigned int view = viewable-1;
    if ((currentTop + view) == items.size()-1){
        //currentTop = right;
    } else if (cursors[cursor] >= (currentTop + view - scrollOffset)){
        currentTop = cursors[cursor] - view + scrollOffset;
    }
}

GridSelect::GridSelect(){
}
GridSelect::~GridSelect(){
}
void GridSelect::act(){
}
void GridSelect::render(const Graphics::Bitmap &, const Font &) const{
}
void GridSelect::addItem(const Util::ReferenceCount<SelectItem> &){
}
void GridSelect::addItems(const std::vector<Util::ReferenceCount<SelectItem> > &){
}
const std::vector<Util::ReferenceCount<SelectItem> > & GridSelect::getItems() const{
    return items;
}
void GridSelect::clearItems(){
}
void GridSelect::setCellDimensions(int width, int height){
}
void GridSelect::setCellSpacing(int x, int y){
}
void GridSelect::setCellMargins(int x, int y){
}
void GridSelect::setCursors(int total){
}
int GridSelect::totalCursors() const{
    return cursors.size();
}
void GridSelect::setCurrentIndex(int cursor, unsigned int location){
}
unsigned int GridSelect::getCurrentIndex(int cursor) const{
    return cursors[cursor];
}
bool GridSelect::up(int cursor){
    return false;
}
bool GridSelect::down(int cursor){
    return false;
}
bool GridSelect::left(int cursor){
    return false;
}
bool GridSelect::right(int cursor){
    return false;
}