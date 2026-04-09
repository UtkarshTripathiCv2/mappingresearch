float offsetX = 0, offsetY = 0;

float x = event.magnetic.x - offsetX;
float y = event.magnetic.y - offsetY;

float heading = atan2(y, x);
