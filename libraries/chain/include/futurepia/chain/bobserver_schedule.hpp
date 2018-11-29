#pragma once

namespace futurepia { namespace chain {

class database;

void update_bobserver_schedule( database& db );
void reset_virtual_schedule_time( database& db );

} }
