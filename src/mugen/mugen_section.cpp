#include "mugen_section.h"
#include "mugen_item_content.h"

MugenItemContent MugenSection::empty;

MugenSection::MugenSection( MugenSection const & copy ) :
header("empty"){
  this->header = copy.header;
  this->itemContent = copy.itemContent;
  reset();
}

MugenSection::MugenSection() :
header( "empty" ){
}

MugenSection::~MugenSection(){
}
    
MugenSection & MugenSection::operator=( MugenSection & s){
  this->header = s.header;
  this->itemContent = s.itemContent;
  reset();
  
  return *this;
}

MugenSection & MugenSection::operator<<( const MugenItemContent & item ) throw( MugenException ){
  if ( header == "empty" ) throw MugenException("This section has no header, cannot add items!");
  itemContent.push_back( item );
  itemContentQueue.push( &itemContent.back() );
  
  return *this;
  
}
    
const MugenItemContent & MugenSection::getNext(){
  if( itemContent.empty() )return empty;
  const MugenItemContent *temp = itemContentQueue.front();
  itemContentQueue.pop();
  return *temp;
}

void MugenSection::reset(){
  while( !itemContentQueue.empty() )itemContentQueue.pop();
  for ( std::vector< MugenItemContent >::iterator begin = itemContent.begin() ; begin != itemContent.end() ; ++begin ){
    this->itemContentQueue.push( &itemContent.back() );
  }
}

void MugenSection::clear(){
  while( !itemContentQueue.empty() )itemContentQueue.pop();
  itemContent.clear();
  header = "empty";
}

