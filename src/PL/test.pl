PROGRAM main;
  VAR
    i, j: integer;
    w : real;
  BEGIN
	  i := 5;
    w := 4;
    call write(i); 
    
    for i := 1 to i < 10 do
    BEGIN
      i += 1;
    END;
    call write(i);

    while i < 15 do
    BEGIN
      i += 1;
    END;
    call write(i);

    repeat
      i += 1
    until i < 20;    
    call write(i);

    case i of    
      10 : i := 11;
      20 : i := 12;
      30 : i := 13;
      40 : i := 14;
    END;
    call write(i);

  END.