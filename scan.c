void do_scan(struct char_data *ch, char *argument, int cmd)
{
  char buf[MAX_STRING_LENGTH];
  int i,newroom;
  static char *keywords[]= { 
    "North", "East", "South", "West", "Up", "Down"
  };

  if(GET_POS(ch) < POSITION_SLEEPING){
    send_to_char("You can't see anything but stars!\n\r",ch);
    return;
  }
  if(GET_POS(ch) == POSITION_SLEEPING){
    send_to_char("You must be dreaming.\n\r",ch);
    return;
  }
  if(IS_AFFECTED(ch, AFF_BLIND)){
    send_to_char("You can't see a thing, you're blind!\n\r", ch);
    return;
  }
  for(i=0;i<6;i++){
    if(EXIT(ch, i)) {
      sprintf(buf,"%s:\n\r",keyword[i]);
      send_to_char(buf,ch);
      if(IS_DARK(ch->in_room) && (!OMNI(ch)) && (!CANINFRA(ch))){
        send_to_char("Dark!\n\r",ch);
        continue;
      }
      if(CAN_GO(ch,i)){
        newroom=world[ch->in_room].dir_option[i]->to_room;
        list_obj_to_char(world[newroom].contents, ch, 0,FALSE);
        list_char_to_char(world[newroom].people,ch,0);
      }
    }
  }
}
