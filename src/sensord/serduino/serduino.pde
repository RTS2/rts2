/* 
 * Driver for Arduino used as multipurpose sensor.
 * Copyright (C) 2010 Petr Kubanek, Insitute of Physics <kubanek@fzu.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#define SIZE_A  30

// array for average values of sensors
int avals[6][SIZE_A];
double asums[6];
int avals_index = 0;

void setup()
{
 // must switch to external reference
 analogReference(EXTERNAL);
  
 Serial.begin(9600);
  
 int i;
  
 for (i = 8; i < 11; i++)
  pinMode(i, INPUT);
 // fill sensor array
 for (i = 0; i < 6; i++)
 {
  asums[i] = 0;
 }
 for (i=0; i < SIZE_A; i++)
 {
  for (int j = 0; j < 6; j++)
  {
   avals[j][i] = analogRead(j);
   asums[j] += avals[j][i];
  }
 }
 avals_index = 0;
}

void sendSensor()
{
 int sensorValue = 0;
 int i;
 for (i = 8; i < 11; i++)
 {
  sensorValue = sensorValue << 1;
  sensorValue |= digitalRead(i);
 }
 Serial.print(sensorValue, DEC);
 Serial.print(" ");
  
 for (i = 0; i < 6; i++)
 {
 
  Serial.print(asums[i] / SIZE_A, DEC);
  Serial.print(" ");
 }
 Serial.println(" ");
}

void loop()
{
 if (Serial.available())
 {
  char ch = Serial.read();
  switch (ch)
  {
   case '?':
    sendSensor();
    break;
   default:
    Serial.println("E unknow command");
  }   
 }
 for (int i = 0; i < 6; i++)
 {
  asums[i] -= avals[i][avals_index];
  int r = analogRead(i);
  avals[i][avals_index] = r;
  asums[i] += r;
 }
 avals_index ++;
 avals_index %= SIZE_A;
}
