
void do_finger(struct char_data *ch, char *argument, int cmd)
{
  struct char_data *get_player_vis(struct char_data *ch, char *name);
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  char name[KBYTE];
  time_t last_logon;
  struct char_file_u cfu;

  one_argument(argument, name);
  k = get_player_vis(ch, name);
  if (k) {
    send_to_char("On line now.\n\r",ch);
  } else {
    if(load_char(name,&cfu) >= 0){
      sprintf(buf, "Name: %s\n\rLevel: %d\n\rLast Logon: %s\r",
        cfu.name,
        cfu.level,
        (char *) ctime((time_t *) & cfu.last_logon)
      send_to_char(buf,ch);
    } else {
      send_to_char("Who dat?\n\r",ch);
    }
  }
}
