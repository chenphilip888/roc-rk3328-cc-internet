server: clean_server
	gcc -g server_led.c -o server_led
	gcc -g server_lcd.c -o server_lcd
	gcc -g server_servo.c -o server_servo

client: clean_client
	gcc -g client_led.c -o client_led
	gcc -g client_lcd.c -o client_lcd
	gcc -g client_servo.c -o client_servo

clean_server:
	\rm -f server_led server_lcd server_servo

clean_client:
	\rm -f client_led client_lcd client_servo

clean:
	\rm -f server_led server_lcd server_servo client_led client_lcd client_servo
