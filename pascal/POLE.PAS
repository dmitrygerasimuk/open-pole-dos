program Pole;
{ Source encoding: CP866. }

{$F-}
{$M 16000,0,655360}
{$S-}{$R-}{$I+}{$B-}{$O+}{$D-}{$L-}

uses Crt,Dos,Graph;

type
  TPerson=record
    PicNo:word;
    Name:string[10];
  end;

const
  Persons:array[1..6] of TPerson=
   ((PicNo:21;Name:'ПЯТАЧОК'),
    (PicNo:22;Name:'ВИННИ-ПУХ'),
    (PicNo:48;Name:'КРОЛИК'),
    (PicNo:49;Name:'ИА-ИА'),
    (PicNo:50;Name:'КАРЛСОН'),
    (PicNo:51;Name:'СОВА'));

  BarabanPic:array[0..15] of integer=
    (4,5,12,6,14,7,13,6,12,8,11,5,10,7,9,8);

  BarabanVal:array[0..11] of integer=
    (100,5,10,15,20,-1,202,204,0,300,25,50);

  IgrokX:array[1..3] of integer=(388,82,320);
  IgrokY:array[1..3] of integer=(120,120,294);
  OblakoX:array[1..3] of integer=(410,130,240);

  Alfavit:string='АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ';

  Prizy:array[0..9] of string[24]=
   ('ЗУБНУЮ ЩЕТКУ','Порошок ARIEL','Расчестку для усов',
    'Зубочистку','Круг для унитаза','Рулон мягкой бумаги',
    'Шнурки для калош','Подтяжки для носков','Беруши для ушей',
    'Пивную открывашку');

  LatKeys:string[33]='QWERTYUIOP{}ASDFGHJKL;''ZXCVBNM,./';
  RusKeys:string[33]='ЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮЁ';

type
  TSlovo=string[20];
  TBuf=array[0..$7FFF] of byte;
  PBuf=^TBuf;

  TRecord=record
    Name:string[10];
    Score:word;
  end;

var
  Sostav:array[1..6] of integer;
  Rekordy:array[1..8] of TRecord;
  Result,Vernyh,Sovpalo:integer;
  Otkryto:array[1..20] of integer;
  Pozicii:array[1..20] of integer;
  Nazvano:array[0..31] of integer;
  Otvet:string[20];
  Slovo,Tema:TSlovo;
  PrevSlova:array[0..2] of integer;
  Unused_06B0:string;
  Bukva:char;
  Unused_07B2:integer;
  Mouse,Tmp2:integer;
  Unused_07B8:Registers;
  FPic:file of TRecord;
  Unused_084C:integer;
  Ochki:array[0..3] of integer;
  Igrok,Tur:integer;
  K:word;
  Tmp:integer;
  Unused_085E,Unused_0860:integer;
  Zvuk:integer;
  Text:string;
  Key:char;
  Unused_0966,Unused_0968:integer;
  GM,GD,I:integer;
  BarPos,Sector:integer;
  Unused_0974:File;
  R:Registers;
  Pic:array[0..122] of pointer;
  Font1,Font2,Font3:PBuf;
  Buf:TBuf absolute $A000:$7D00;
  Video:TBuf absolute $A000:0;

procedure PutPic(var Source,Dest; X,Y,Transparent:word); external;
{$L RLE.OBJ}

procedure EgaVgaDriverProc; external;
{$L EGAVGA.OBJ}

procedure Quit;
var Unused:integer;
begin
  RestoreCrtMode;
  Halt(0);
end;

procedure LoadFiles;
var
  Head:array[0..127] of byte;
  Name:string[255];
  F:file;
  I,N:integer;
begin
  Name:='pole.lib';
  Assign(F,Name);
  {$I-} Reset(F,128); {$I+}
  if IOResult<>0 then begin
    WriteLn('File '+Name+' not found');
    Halt(1);
  end;
  BlockRead(F,Head,1);
  for I:=1 to Head[0] do begin
    N:=Head[I];
    GetMem(Pic[I-1],N shl 7);
    BlockRead(F,Pic[I-1]^,N);
  end;
  Close(F);

  Name:='pole.fnt';
  Assign(F,Name);
  {$I-} Reset(F,128); {$I+}
  if IOResult<>0 then begin
    WriteLn('File '+Name+' not found');
    Halt(1);
  end;
  GetMem(Font1,$0800); BlockRead(F,Font1^,12,I);
  GetMem(Font2,$0800); BlockRead(F,Font2^,16,I);
  GetMem(Font3,$0E00); BlockRead(F,Font3^,28,I);
  Close(F);

  Name:='pole.pic';
  Assign(FPic,Name);
  {$I-} Reset(FPic); {$I+}
  if IOResult<>0 then begin
    for I:=1 to 8 do with Rekordy[I] do begin
      if Odd(I) then
        Name:=#$8B#$88#$91#$92#$9C#$85#$82
      else
        Name:=#$9F#$8A#$93#$81#$8E#$82#$88#$97;
      Score:=135-I*15;
    end;
  end else begin
    for I:=1 to 8 do Read(FPic,Rekordy[I]);
    Close(FPic);
  end;
end;

procedure Print(S:string; X,Y:integer; Color:word;
                Height,Step:integer; var Dest:TBuf);
var
  Row,Glyph,Code,I,J:word;
  XB:integer;
  Bit,Sh,Bits,Dummy:byte;
begin
  XB:=X shr 3;
  Bit:=X and 7;
  Sh:=8-Bit;
  Row:=Y*80+XB;
  Port[$3CE]:=5; Port[$3CF]:=2;
  for I:=1 to Length(S) do begin
    XB:=X shr 3;
    Bit:=X and 7;
    Sh:=8-Bit;
    Row:=Y*80+XB;
    Code:=Ord(S[I]);
    Glyph:=(Height+1)*Code;
    for J:=0 to Height do begin
      if Height=5 then Bits:=Font1^[Glyph+J]
      else if Height=7 then Bits:=Font2^[Glyph+J]
      else Bits:=Font3^[Glyph+J];
      Port[$3CE]:=8;
      Port[$3CF]:=Bits shr Bit;
      Dummy:=Dest[J*80+Row];
      FillChar(Dest[J*80+Row],1,Color);
      if Sh<>8 then begin
        Port[$3CE]:=8;
        Port[$3CF]:=Bits shl Sh;
        Dummy:=Dest[J*80+Row+1];
        FillChar(Dest[J*80+Row+1],1,Color);
      end;
    end;
    X:=X+Step;
  end;
end;

procedure InitVideo;
begin
  if RegisterBGIDriver(@EgaVgaDriverProc)<0 then Halt(0);
  GD:=3; GM:=1;
  InitGraph(GD,GM,'');
  SetPalette(6,$2E);
  SetPalette(4,$14);
  SetPalette(5,$27);
  R.AX:=0; Intr($33,R);
  if R.AX=0 then Mouse:=0 else Mouse:=1;
  if Mouse>0 then begin
    R.AX:=2; Intr($33,R);
    R.AX:=7; R.CX:=0; R.DX:=639; Intr($33,R);
  end;
end;

procedure FillEga(Ofs:word; LastRow:integer; Count,Color:word);
var I:integer;
begin
  Port[$3CE]:=5; Port[$3CF]:=2;
  Port[$3CE]:=8; Port[$3CF]:=$FF;
  FillChar(Video[Ofs],1,Color);
  Port[$3CE]:=5; Port[$3CF]:=1;
  Move(Video[Ofs],Video[Ofs],1);
  for I:=0 to LastRow do begin
    FillChar(Video[Ofs],Count,1);
    Inc(Ofs,80);
  end;
end;

procedure MoveVideo(Source,Dest:word; LastRow:integer; Count:word);
var I:integer;
begin
  Port[$3CE]:=5; Port[$3CF]:=1;
  for I:=0 to LastRow do begin
    Move(Video[Source],Video[Dest],Count);
    Inc(Source,80); Inc(Dest,80);
  end;
end;

procedure ClearKeys;
begin
  while KeyPressed do Key:=ReadKey;
  if Mouse>0 then repeat
    R.AX:=3; Intr($33,R);
  until R.BX=0;
end;

procedure Boss;
var
  Pal:PaletteType;
  I:integer;
begin
  GetPalette(Pal);
  for I:=0 to 15 do SetPalette(I,0);
  ClearKeys;
  repeat
    R.BX:=0;
    if Mouse>0 then begin
      R.AX:=3; Intr($33,R);
    end;
    Key:=#7;
    if KeyPressed then Key:=ReadKey;
    if Key=#27 then Quit;
    if Key=#19 then Zvuk:=1-Zvuk;
  until (Key=' ') or (R.BX=1);
  Key:=#7;
  SetAllPalette(Pal);
end;

procedure WaitKey(Count:integer);
begin
  ClearKeys;
  repeat
    if Count>0 then Dec(Count);
    Delay(50);
    R.BX:=0;
    if Mouse>0 then begin
      R.AX:=3; Intr($33,R);
    end;
    Key:=#7;
    if KeyPressed then Key:=ReadKey;
    if Key=#27 then Quit;
    if Key=#19 then Zvuk:=1-Zvuk;
    if Key=#9 then Boss;
  until (Key=' ') or (R.BX=1) or (Count=1);
end;

procedure Beep6;
var I:integer;
begin
  for I:=1 to 6 do begin
    Sound(Zvuk*Random(100));
    Delay(6-I);
    NoSound;
    Delay(1);
  end;
end;

procedure YakAnim;
var Ofs:word;
begin
  Ofs:=$2BFB;
  MoveVideo(Ofs+$7D00,Ofs,190,21);
  Beep6;
  PutPic(Pic[45]^,Video,511,173,16);
  Delay(30);
  PutPic(Pic[43]^,Video,532,209,16);
end;

procedure DrawScore(N:integer);
var
  I,J:integer;
  S:string;
begin
  FillEga((IgrokY[N]-1)*80+(IgrokX[N]-4) div 8,29,13,7);
  for I:=1 to Ochki[N] do begin
    Sound(Zvuk*(Random(10)+50));
    PutPic(Pic[30]^,Video,IgrokX[N]-4+Random(12),
           IgrokY[N]-1+Random(7),1);
    NoSound;
  end;
  if Ochki[N]>0 then begin
    PutPic(Pic[30]^,Video,IgrokX[N],IgrokY[N],1);
    Str(Ochki[N],S);
    for I:=-1 to 1 do
      for J:=-1 to 1 do
        Print(S,IgrokX[N]+34-Length(S)*4+I,
              IgrokY[N]+4+J,0,13,8,Video);
    Print(S,IgrokX[N]+34-Length(S)*4,IgrokY[N]+4,15,13,8,Video);
  end else
    PutPic(Pic[28]^,Video,IgrokX[N]+4,IgrokY[N]+10,2);
end;

procedure YakFace;
begin
  Beep6;
  PutPic(Pic[43]^,Video,532,201,16);
  Delay(100);
  PutPic(Pic[44]^,Video,532,201,16);
  Delay(100);
end;

procedure YakEyes;
var I:integer;
begin
  Beep6;
  PutPic(Pic[44]^,Video,532,209,16); Delay(200);
  PutPic(Pic[43]^,Video,532,209,16); Delay(150);
  PutPic(Pic[44]^,Video,532,209,16); Delay(150);
  PutPic(Pic[42]^,Video,511,173,16);
  PutPic(Pic[44]^,Video,532,201,16);
  for I:=1 to 3 do begin
    Beep6;
    PutPic(Pic[45]^,Video,511,173,16);
    PutPic(Pic[43]^,Video,532,209,16);
    Delay(Random(2)*200+100);
    PutPic(Pic[42]^,Video,511,173,16);
    Delay(Random(2)*50+100);
  end;
end;

procedure Yak(N:integer);
var
  Ofs:word;
  S,S1,S2:string[20];
begin
  YakEyes;
  Ofs:=$2BFB;
  MoveVideo(Ofs,Ofs+$7D00,190,21);
  PutPic(Pic[35]^,Video,476,142,1);

  if N=1 then begin
    case Igrok of
      1:S:='Первый';
      2:S:='Второй';
      3:S:='Третий';
    end;
    S1:=S+' игрок!';
    S2:='Вращайте барабан!';
  end;

  if N=2 then begin
    if (Ochki[0]>0) and (Ochki[0]<100) then begin
      Str(Ochki[0],S);
      S1:='У Вас '+S+' очков!';
      S2:='Назовите букву!';
    end;
    if Ochki[0]=0 then begin
      S1:='У Вас 0 очков..';
      S2:='Увы! Переход хода..';
    end;
    if Ochki[0]<0 then begin
      S1:='Все деньги сгорели!';
      S2:='Увы! Переход хода..';
    end;
    if Ochki[0] div 100=2 then begin
      S1:='Деньги удваиваются!';
      S2:='Назовите букву!';
    end;
    if Ochki[0]=100 then begin
      S1:='Сектор ПЛЮС!';
      S2:='Открой любую букву!';
    end;
    if Ochki[0]=300 then begin
      S1:='Сектор ПРИЗ!';
      S2:='ПРИЗ или играем?';
    end;
  end;

  if N=3 then begin
    if Sovpalo>0 then begin
      S2:='Есть такая буква!';
      S1:='Браво!!';
    end else begin
      S1:='Нет такой буквы!';
      S2:='Переход хода..';
    end;
  end;

  if N=4 then begin
    case Igrok of
      1:S:='Первый';
      2:S:='Второй';
      3:S:='Третий';
    end;
    S1:=S+' игрок';
    S2:='выиграл раунд!';
  end;

  if N=5 then begin
    S1:='За 3 буквы ПРЕМИЯ!';
    S2:='Внесите шкатулки!';
  end;
  if N=6 then begin
    S1:='Где деньги?';
    S2:='Выбирайте!';
  end;
  if N=7 then begin
    S1:='Браво!!!';
    S2:='Вы отгадали!';
  end;
  if N=8 then begin
    S1:='Увы! Эта';
    S2:='шкатулка пуста!';
  end;
  if N=9 then begin
    S2:='Загадано слово:';
    S1:='Начинаем игру!';
  end;
  if N=10 then begin
    S1:='Тема:';
    S2:=Tema;
  end;
  if N=11 then begin
    S1:='Вы совершенно';
    S2:='правы!!';
  end;
  if N=12 then begin
    S1:='Неправильно! Вы';
    S2:='покидаете игру!';
  end;
  if N=13 then begin
    S1:='Поздравляю! Вы';
    S2:='выиграли финал!';
  end;
  if N=14 then begin
    S1:='К сожалению Вы';
    S2:='покидаете игру..';
  end;
  if N=15 then begin
    S1:='Если так, то';
    S2:='назовите букву.';
  end;
  if N=16 then begin
    S1:='РЕКЛАМНАЯ';
    S2:='ПАУЗА!';
  end;
  if N=17 then begin
    S1:='ПРИЗ или';
    S2:=Text+' рублей?';
  end;
  if N=18 then begin
    S1:='Забирайте';
    S2:='свои ДЕНЬГИ!';
  end;
  if N=19 then begin
    S1:='Представляю';
    S2:='участников!';
  end;
  if N=20 then begin
    S1:='Забирайте';
    S2:='свой ПРИЗ!';
  end;

  Print(S1,554-Length(S1)*4,145,0,13,8,Video);
  Print(S2,554-Length(S2)*4,158,0,13,8,Video);
  YakFace;
end;

procedure GirlRun;
var
  LetterOfs,TempOfs,CurOfs,OldOfs:word;
  LetterX,X,Y,Step,Phase,Timer,PicNo,PosNo,StopX:integer;
begin
  NoSound;
  PosNo:=1;
  StopX:=(Pozicii[PosNo]+5)*16+110;
  MoveVideo(0,$7D00,120,80);
  TempOfs:=$BB80;
  X:=40; Y:=25;
  Timer:=0; Phase:=0;
  CurOfs:=X shr 3+Y*80;
  OldOfs:=CurOfs;

  for Step:=20 to 100 do begin
    Sound(Zvuk*Step);
    Delay(1); NoSound;
    Delay((100-Step) div 5);
  end;

  repeat
    CurOfs:=X shr 3+Y*80;
    Inc(Phase);

    if Timer=10 then begin
      MoveVideo(LetterOfs,LetterOfs+1600,19,2);
      Print(Bukva,LetterX,38,0,13,8,Buf);
    end;

    MoveVideo(CurOfs+$7D00,TempOfs,90,6);

    if Timer>0 then PicNo:=23
    else case Phase mod 4 of
      0:begin PicNo:=24; Step:=10 end;
      1:begin PicNo:=26; Step:=0 end;
      2:begin PicNo:=25; Step:=12 end;
      3:begin PicNo:=26; Step:=3 end;
    end;

    if Timer>0 then Inc(Timer);
    if Timer>20 then Timer:=0;

    PutPic(Pic[PicNo]^,Buf,X,Y,2);
    MoveVideo(CurOfs+$7D00,CurOfs,90,6);
    MoveVideo(OldOfs+$7D00,OldOfs,90,6);
    OldOfs:=CurOfs;

    if Timer=0 then Sound(Zvuk*(Random(100)+1000));
    MoveVideo(TempOfs,OldOfs+$7D00,90,6);
    NoSound;

    if Timer=0 then X:=X+Step;

    if X>StopX then begin
      LetterOfs:=$81B0+Pozicii[PosNo]*2+25;
      LetterX:=(Pozicii[PosNo]+5)*16+125;
      Inc(PosNo);
      if PosNo>Sovpalo then StopX:=1000
      else StopX:=(Pozicii[PosNo]+5)*16+110;
      Timer:=1;
    end;

    Delay(50);
    Key:=#7;
    if KeyPressed then Key:=ReadKey;
    if Key=#19 then Zvuk:=1-Zvuk;
    if Key=#9 then Boss;
    if Key=#27 then Quit;
  until X>582;

  MoveVideo(CurOfs+$7D00,CurOfs,91,8);

  for Step:=100 downto 20 do begin
    Sound(Zvuk*Step);
    Delay((100-Step) div 10);
    NoSound;
    Delay(1);
  end;

  if Ochki[0]<100 then
    Ochki[Igrok]:=Ochki[Igrok]+Ochki[0]*Sovpalo
  else if Ochki[0] div 100=2 then
    Ochki[Igrok]:=Ochki[Igrok]*(Ochki[0] mod 100);

  DrawScore(Igrok);
end;

procedure Letter;
var
  TempOfs,CurOfs,OldOfs:word;
  X,Y,I,J,N,Sel,Ok:integer;
begin
  if Igrok=3 then begin
    CurOfs:=$59B0;
    MoveVideo(CurOfs,CurOfs+$7D00,60,80);

    if Mouse>0 then begin
      R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
    end;

    Sel:=0; Ok:=0;
    ClearKeys;
    TempOfs:=$BB80;
    X:=Sel*20; Y:=314;
    CurOfs:=X shr 3+Y*80;
    OldOfs:=CurOfs;

    repeat
      CurOfs:=X shr 3+Y*80;
      MoveVideo(CurOfs+$7D00,TempOfs,26,3);
      PutPic(Pic[29]^,Buf,X,Y,2);
      MoveVideo(CurOfs+$7D00,CurOfs,26,3);
      MoveVideo(OldOfs+$7D00,OldOfs,26,3);
      OldOfs:=CurOfs;
      MoveVideo(TempOfs,OldOfs+$7D00,26,3);

      Key:=#7;
      if KeyPressed then Key:=ReadKey;
      if Key=#19 then Zvuk:=1-Zvuk;
      if Key=#9 then Boss;

      if Key=#0 then begin
        Key:=ReadKey;
        if Key=#$4B then Dec(Sel);
        if Key=#$4D then Inc(Sel);
      end;

      if Mouse>0 then begin
        R.AX:=3; Intr($33,R);
        if R.CX<305 then begin
          Dec(Sel);
          R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
        end;
        if R.CX>335 then begin
          Inc(Sel);
          R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
        end;
      end;

      if Sel<0 then Sel:=0;
      if Sel>31 then Sel:=31;
      X:=Sel*20; Y:=314;

      if Key=#27 then Quit;

      if (Key=' ') or ((Mouse>0) and (R.BX=1)) then begin
        if Nazvano[Sel]=0 then Ok:=1
        else begin
          Sound(Zvuk*1000);
          Delay(50);
          NoSound;
        end;
      end;
      Delay(30);
    until Ok>0;

    Sound(Zvuk*100);
    MoveVideo(CurOfs+$7D00,CurOfs,26,3);
    NoSound;
  end else begin
    J:=0;
    N:=Length(Slovo);
    for I:=1 to N do
      if Otkryto[I]=0 then Inc(J);

    if (J*2>N) and (J>1) then X:=0
    else if Random(Tur+2)=0 then X:=0
    else X:=1;

    if X=0 then repeat
      Sel:=Random(32);
    until Nazvano[Sel]=0
    else begin
      repeat
        J:=Random(N)+1;
      until Otkryto[J]=0;
      Key:=Slovo[J];
      Sel:=0;
      while Alfavit[Sel+1]<>Key do Inc(Sel);
    end;

    CurOfs:=OblakoX[Igrok] div 8+$2260;
    MoveVideo(CurOfs,CurOfs+$7D00,40,20);
    PutPic(Pic[39]^,Video,OblakoX[Igrok],110,2);
    Print(#$81#$E3#$AA#$A2#$A0'  '+Alfavit[Sel+1]+'!',
          OblakoX[Igrok]+6,118,0,13,8,Video);
    WaitKey(40);
    MoveVideo(CurOfs+$7D00,CurOfs,40,20);
  end;

  Nazvano[Sel]:=1;
  X:=Sel*20; Y:=332;

  for I:=0 to 3 do begin
    if I<3 then PutPic(Pic[I+18]^,Video,X,Y,16)
    else begin
      Port[$3CE]:=5; Port[$3CF]:=0;
      SetFillStyle(1,7);
      Bar(X,Y,X+19,Y+18);
    end;
    for J:=1 to 10 do begin
      Sound((J*10+I*100+50)*Zvuk);
      Delay(1); NoSound;
      Delay(I*4+J div 5);
    end;
  end;

  Bukva:=Alfavit[Sel+1];
  Sovpalo:=0;

  for I:=1 to Length(Slovo) do
    if (Slovo[I]=Bukva) and (Otkryto[I]=0) then begin
      Inc(Sovpalo);
      Pozicii[Sovpalo]:=I;
      Otkryto[I]:=1;
    end;

  Yak(3);
  WaitKey(40);
  YakAnim;
end;

procedure PlusLetter;
var
  TempOfs,CurOfs,OldOfs:word;
  X,Y,I,J,N,Sel,Ok:integer;
  S:string[2];
begin
  if Igrok=3 then begin
    CurOfs:=$0320;
    MoveVideo(CurOfs,CurOfs+$7D00,60,80);

    if Mouse>0 then begin
      R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
    end;

    Sel:=1; Ok:=0;
    ClearKeys;
    TempOfs:=$BB80;
    X:=Sel*16+200; Y:=12;
    CurOfs:=X shr 3+Y*80;
    OldOfs:=CurOfs;

    repeat
      CurOfs:=X shr 3+Y*80;
      MoveVideo(CurOfs+$7D00,TempOfs,26,3);
      PutPic(Pic[29]^,Buf,X,Y,2);
      MoveVideo(CurOfs+$7D00,CurOfs,26,3);
      MoveVideo(OldOfs+$7D00,OldOfs,26,3);
      OldOfs:=CurOfs;
      MoveVideo(TempOfs,OldOfs+$7D00,26,3);

      Key:=#7;
      if KeyPressed then Key:=ReadKey;
      if Key=#19 then Zvuk:=1-Zvuk;
      if Key=#9 then Boss;

      if Key=#0 then begin
        Key:=ReadKey;
        if Key=#$4B then Dec(Sel);
        if Key=#$4D then Inc(Sel);
      end;

      if Mouse>0 then begin
        R.AX:=3; Intr($33,R);
        if R.CX<305 then begin
          Dec(Sel);
          R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
        end;
        if R.CX>335 then begin
          Inc(Sel);
          R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
        end;
      end;

      if Sel<1 then Sel:=1;
      if Length(Slovo)<Sel then Sel:=Length(Slovo);
      X:=Sel*16+200; Y:=12;

      if Key=#27 then Quit;

      if (Key=' ') or ((Mouse>0) and (R.BX=1)) then begin
        if Otkryto[Sel]=0 then Ok:=1
        else begin
          Sound(Zvuk*1000);
          Delay(50);
          NoSound;
        end;
      end;
      Delay(30);
    until Ok>0;

    Sound(Zvuk*100);
    MoveVideo(CurOfs+$7D00,CurOfs,26,3);
    NoSound;
  end else begin
    repeat
      Sel:=Random(Length(Slovo))+1;
    until Otkryto[Sel]=0;

    CurOfs:=OblakoX[Igrok] div 8+$2260;
    MoveVideo(CurOfs,CurOfs+$7D00,40,20);
    PutPic(Pic[39]^,Video,OblakoX[Igrok],110,2);
    Str(Sel,S);
    Print(S+'-я буква',OblakoX[Igrok]+6,118,0,13,8,Video);
    WaitKey(40);
    MoveVideo(CurOfs+$7D00,CurOfs,40,20);
  end;

  Bukva:=Slovo[Sel];
  Sovpalo:=0;

  for I:=1 to Length(Slovo) do
    if (Slovo[I]=Bukva) and (Otkryto[I]=0) then begin
      Inc(Sovpalo);
      Pozicii[Sovpalo]:=I;
      Otkryto[I]:=1;
    end;

  I:=0;
  repeat Inc(I) until Bukva=Alfavit[I];
  I:=I-1;
  Nazvano[I]:=1;
  X:=I*20; Y:=332;

  for I:=0 to 3 do begin
    if I<3 then PutPic(Pic[I+18]^,Video,X,Y,16)
    else begin
      Port[$3CE]:=5; Port[$3CF]:=0;
      SetFillStyle(1,7);
      Bar(X,Y,X+19,Y+18);
    end;
    for J:=1 to 10 do begin
      Sound((J*10+I*100+50)*Zvuk);
      Delay(1); NoSound;
      Delay(I*4+J div 5);
    end;
  end;
end;

function ToRus(Key1:char):char;
var
  C:char;
  I:integer;
begin
  C:=' ';
  Key1:=UpCase(Key1);

  if Key1<#$80 then
    for I:=1 to 33 do
      if LatKeys[I]=Key1 then C:=RusKeys[I];

  if (Key1>=#$80) and (Key1<#$A0) then C:=Key1;
  if (Key1>=#$A0) and (Key1<#$B0) then C:=Chr(Ord(Key1)-$20);
  if (Key1>=#$E0) and (Key1<#$F0) then C:=Chr(Ord(Key1)-$50);

  ToRus:=C;
end;

function EnterWord:integer;
var
  Ofs:integer;
  I,Bad,Count:integer;
  S:string;
  Unused:integer;
begin
  Port[$3CE]:=5; Port[$3CF]:=0;
  SetColor(0);

  for I:=0 to Length(Slovo)-1 do
    Rectangle(I*16+4,300,I*16+20,320);

  Count:=0; S:='';
  Ofs:=$5EB1;
  I:=0;

  repeat
    Key:=#7;
    if KeyPressed then Key:=ReadKey;
    if Key=#19 then Zvuk:=1-Zvuk;
    if Key=#9 then Boss;
    if Key=#27 then Quit;

    if (Key=#8) and (Count>0) then begin
      Delete(S,Count,1);
      FillEga(Count*2+Ofs,13,1,7);
      Dec(Count);
      FillEga(Count*2+Ofs,13,1,7);
    end;

    if Key>' ' then Key:=ToRus(Key);

    if (Key>' ') and (Length(Slovo)>Count) then begin
      FillEga(Count*2+Ofs,13,1,7);
      Print(Key,Count*16+8,303,0,13,8,Video);
      S:=S+Key;
      Inc(Count);
    end;

    Delay(25);
    Inc(I);
    if I>39 then I:=0;
    FillEga(Count*2+Ofs+800,2,1,(I div 20)*7);
  until Key=#13;

  Otvet:=S;
  FillEga($5DC0,20,Length(Slovo)*2+3,7);

  Bad:=0;
  if Length(Otvet)<>Length(Slovo) then Bad:=1
  else
    for I:=1 to Length(Slovo) do
      if Otvet[I]<>Slovo[I] then Bad:=1;

  if Bad>0 then EnterWord:=1 else EnterWord:=0;
end;

procedure EnterName(Place:integer);
var
  Ofs,I,Unused,Count:integer;
  S:string;
  UnusedTail:integer;
begin
  Count:=0; S:='';
  Ofs:=(Place*14+195)*80+62;
  I:=0;

  repeat
    Key:=#7;
    if KeyPressed then Key:=ReadKey;
    if Key=#19 then Zvuk:=1-Zvuk;
    if Key=#9 then Boss;
    if Key=#27 then Quit;

    if (Key=#8) and (Count>0) then begin
      Delete(S,Count,1);
      FillEga(Ofs+Count,13,1,0);
      Dec(Count);
      FillEga(Ofs+Count,13,1,0);
    end;

    if Key>' ' then Key:=ToRus(Key);

    if (Key>#31) and (Count<10) then begin
      FillEga(Ofs+Count,13,1,0);
      Print(Key,Count*8+497,Place*14+195,7,13,8,Video);
      Print(Key,Count*8+496,Place*14+195,15,13,8,Video);
      S:=S+Key;
      Inc(Count);
    end;

    Delay(25);
    Inc(I);
    if I>39 then I:=0;
    FillEga(Ofs+Count+800,2,1,(I div 20)*11);
  until Key=#13;

  Rekordy[Place].Name:=S;
end;

procedure NewWord;
var
  I,N:integer;
  F:file of TSlovo;
begin
  Assign(F,'pole.ovl');
  Reset(F);
  Read(F,Tema);
  Val(Tema,N,I);

  repeat
    I:=Random(N)+1;
  until (PrevSlova[0]<>I) and (PrevSlova[1]<>I) and
        (PrevSlova[2]<>I);

  N:=I;
  PrevSlova[Tur]:=I;

  for I:=1 to N do begin
    Read(F,Slovo);
    Read(F,Tema);
  end;
  Close(F);

  for I:=1 to Length(Slovo) do
    Slovo[I]:=Chr(Ord(Slovo[I])-$20);

  for I:=1 to Length(Tema) do
    Tema[I]:=Chr(Ord(Tema[I])-$20);
end;

procedure DrawBaraban;
var
  I,X,Y:integer;
  A:real;
begin
  PutPic(Pic[(BarPos+3) mod 4]^,Video,160,115,3);
  for I:=0 to 15 do begin
    A:=(2*I+BarPos)*Pi/16;
    X:=Round(Cos(A)*84)+261;
    Y:=Round(Sin(A)*63)+196;
    PutPic(Pic[BarabanPic[I]]^,Video,X,Y,7);
  end;
  PutPic(Pic[16]^,Video,267,152,7);
end;

procedure Baraban;
var
  I,Base,Tick,Len,Stage:word;
  X,Y:integer;
  A:real;
begin
  Stage:=0; Len:=0; Tick:=0; Base:=$2404;

  repeat
    I:=0;

    if Stage>0 then case Stage of
      4:begin Inc(BarPos); I:=1 end;
      3:if Tick mod 2=0 then begin I:=1; Inc(BarPos) end;
      2:if Tick mod 3=0 then begin I:=1; Inc(BarPos) end;
      1:if Tick mod 4=0 then begin I:=1; Inc(BarPos) end;
    end;

    if (Stage=0) and (Len>0) and (BarPos mod 2=1) then Inc(BarPos);
    if BarPos>=32 then BarPos:=BarPos-32;

    FillEga(Base+$7D00,172,28,7);
    PutPic(Pic[(BarPos+3) mod 4]^,Buf,160,115,3);

    if I>0 then Sound(Zvuk*(55+(Len-Tick)*10+Stage*100));

    for I:=0 to 15 do begin
      A:=(BarPos+2*I)*Pi/16;
      X:=Round(Cos(A)*84)+261;
      Y:=Round(Sin(A)*63)+196;
      PutPic(Pic[BarabanPic[I]]^,Buf,X,Y,7);
      NoSound;
    end;

    NoSound;
    if Stage>0 then Inc(Tick);

    if Tick>Len then begin
      Len:=Random(10)+5;
      if Stage>0 then Dec(Stage);
      Tick:=0;
    end;

    PutPic(Pic[16]^,Buf,267,152,7);
    MoveVideo(Base+$7D00,Base,172,28);

    Key:=#7;
    if KeyPressed then Key:=ReadKey;
    if Key=#19 then Zvuk:=1-Zvuk;
    if Key=#9 then Boss;

    if Mouse>0 then begin
      R.AX:=3; Intr($33,R);
    end;

    if (Stage=0) and (Len=0) then begin
      if Igrok<3 then WaitKey(50);
      YakAnim;
      Stage:=4;
      Len:=Random(20)+10;
    end;

    if Key=#27 then Quit;
  until (Stage=0) and (Len>0) and (BarPos mod 2=0);

  Sector:=(28-BarPos div 2) mod 16;
  Ochki[0]:=BarabanVal[BarabanPic[Sector]-4];

  for X:=1 to 30 do begin
    Sound((1000-X*30)*Zvuk);
    Delay(X div 5);
    Delay(X div 3);
    NoSound;
  end;
end;

procedure Boxes;
var
  I,Lid,PrizeY,Sel,WinBox:integer;

  procedure SelectBox;
  var
    Base,Done:word;
  begin
    Base:=$4B00;
    Done:=0;

    if Mouse>0 then begin
      R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
    end;

    Sel:=1;
    ClearKeys;

    repeat
      FillEga(Base+$7D00,26,20,7);
      PutPic(Pic[29]^,Buf,Sel*60+18,240,2);
      MoveVideo(Base+$7D00,Base,26,20);

      Key:=#7;
      if KeyPressed then Key:=ReadKey;
      if Key=#19 then Zvuk:=1-Zvuk;
      if Key=#9 then Boss;

      if Key=#0 then begin
        Key:=ReadKey;
        if Key in [#75,#77] then Sel:=1-Sel;
      end;

      if Mouse>0 then begin
        R.AX:=3; Intr($33,R);
        if (R.CX<300) or (R.CX>340) then begin
          Sel:=1-Sel;
          R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
        end;
      end;

      if Key=#27 then Quit;
      if (Key=' ') or ((Mouse>0) and (R.BX=1)) then Done:=1;
      Delay(30);
    until Done>0;

    FillEga(Base,26,20,7);
  end;

begin
  Lid:=5;
  PrizeY:=24;

  FillEga($C800,80,20,7);
  PutPic(Pic[37]^,Buf,6,272-Lid,7);
  PutPic(Pic[37]^,Buf,64,280-Lid,7);
  PutPic(Pic[38]^,Buf,66,280-PrizeY,7);

  for I:=40 downto 0 do begin
    Sound((1000-I*20)*Zvuk);
    MoveVideo($C800,I*2*80+$4B00,80-I*2,20);
    NoSound;
    Delay(I);
  end;

  WaitKey(0);
  FillEga($C800,51,20,7);

  Sound(1000*Zvuk);
  PutPic(Pic[36]^,Buf,6,272,7);
  NoSound;

  Sound(100*Zvuk);
  PutPic(Pic[36]^,Buf,64,280,7);
  NoSound;

  Sound(500*Zvuk);
  MoveVideo($C800,$4B00,80,20);
  NoSound;

  WaitKey(40);

  Lid:=(Random(20)+10)*2;
  WinBox:=1;

  for I:=1 to Lid do begin
    Sound(Zvuk*(Random(100)+50));
    Delay(10); NoSound; Delay(50);

    if I mod 2=0 then WinBox:=1-WinBox;

    FillEga($C800,80,20,7);

    if not Odd(I) then begin
      PutPic(Pic[36]^,Buf,6,272,7);
      PutPic(Pic[36]^,Buf,64,280,7);
    end else begin
      PutPic(Pic[36]^,Buf,40,262,7);
      PutPic(Pic[36]^,Buf,30,290,7);
    end;

    MoveVideo($C800,$4B00,80,20);
  end;

  YakAnim;
  Yak(6);
  SelectBox;
  YakAnim;

  Lid:=5;
  FillEga($C800,80,20,7);

  PutPic(Pic[37-Sel]^,Buf,6,272-(1-Sel)*Lid,7);
  PutPic(Pic[36+Sel]^,Buf,64,280-Lid*Sel,7);

  MoveVideo($C800,$4B00,80,20);

  if WinBox=Sel then begin
    Ochki[Igrok]:=Ochki[Igrok]+100;

    if WinBox=1 then
      PutPic(Pic[38]^,Video,66,280-PrizeY,7)
    else
      PutPic(Pic[38]^,Video,8,272-PrizeY,7);

    Yak(7);
    DrawScore(Igrok);
  end else Yak(8);

  WaitKey(0);
  YakAnim;
  FillEga($4B00,80,20,7);
end;

function Choice(Mode:integer):integer;
var
  I,Unused1,Unused2,Sel,Unused3:integer;

  procedure Select;
  var
    Base,Done:word;
  begin
    Base:=$4B00;
    Done:=0;

    if Mouse>0 then begin
      R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
    end;

    Sel:=1;
    ClearKeys;

    repeat
      FillEga(Base+$7D00,26,20,7);
      PutPic(Pic[29]^,Buf,Sel*60+30,240,2);
      MoveVideo(Base+$7D00,Base,26,20);

      Key:=#7;
      if KeyPressed then Key:=ReadKey;
      if Key=#9 then Boss;
      if Key=#19 then Zvuk:=1-Zvuk;

      if Key=#0 then begin
        Key:=ReadKey;
        if Key in [#75,#77] then Sel:=1-Sel;
      end;

      if Mouse>0 then begin
        R.AX:=3; Intr($33,R);
        if (R.CX<300) or (R.CX>340) then begin
          Sel:=1-Sel;
          R.AX:=4; R.CX:=320; R.DX:=100; Intr($33,R);
        end;
      end;

      if Key=#27 then Quit;
      if (Key=' ') or ((Mouse>0) and (R.BX=1)) then Done:=1;
      Delay(30);
    until Done>0;

    FillEga(Base,26,20,7);
  end;

begin
  FillEga($C800,80,20,7);
  PutPic(Pic[28]^,Buf,2,270,2);
  PutPic(Pic[28]^,Buf,68,270,2);

  if Mode=1 then begin
    Print('Скажу   Кручу',12,290,0,13,8,Buf);
    Print('СЛОВО  БАРАБАН',12,305,0,13,8,Buf);
  end;

  if Mode=2 then begin
    Print('Беру   Буду',18,290,0,13,8,Buf);
    Print('ПРИЗ  ИГРАТЬ',18,305,0,13,8,Buf);
  end;

  if Mode=3 then begin
    Print('Беру    Беру',18,290,0,13,8,Buf);
    Print('ПРИЗ   ДЕНЬГИ',18,305,0,13,8,Buf);
  end;

  for I:=40 downto 0 do begin
    Sound((1000-I*20)*Zvuk);
    MoveVideo($C800,I*2*80+$4B00,80-I*2,20);
    NoSound;
    Delay(I);
  end;

  Select;
  FillEga($4B00,80,20,7);

  if Sel=1 then Choice:=0 else Choice:=Mode;
end;

procedure Intro;
var
  I,J,K1,DX:integer;
begin
  SetFillStyle(1,7);
  SetColor(7);
  SetLineStyle(0,0,3);

  for I:=0 to 160 do begin
    Bar(I+20,345-I,620-I,345-I);
    Delay(10);
  end;

  WaitKey(40);

  for I:=0 to 160 do begin
    Line(20,345,180-I,185-I);
    Bar(180-I,185-I,I+460,185-I);
    Line(I+460,185-I,620,345);
    Delay(5);
  end;

  SetLineStyle(0,0,0);
  SetFillStyle(1,7);
  Bar(20,25,620,345);

  SetFillStyle(1,4);

  for J:=1 to 60 do begin
    for I:=0 to 8 do
      Bar((J-1)*10+20,I*40+25,J*10+20,I*40+27);
    Delay(10);
  end;

  for J:=1 to 40 do begin
    for I:=0 to 12 do
      Bar(I*50+19,(J-1)*8+25,I*50+21,J*8+25);
    Delay(10);
  end;

  WaitKey(50);

  PutPic(Pic[46]^,Video,90,60,7);
  PutPic(Pic[47]^,Video,280,60,7);

  WaitKey(50);

  Text:=' КАПИТАЛШОУ ';

  for K1:=1 to Length(Text) do begin
    if K1<9 then DX:=-15 else DX:=0;

    for I:=-1 to 1 do
      for J:=-1 to 2 do
        Print(Text[K1],K1*50+I+DX,J+238,0,13,8,Video);

    Sound(Zvuk*K1*100);

    for I:=0 to 1 do
      Print(Text[K1],K1*50+DX,I+238,15,13,8,Video);

    NoSound;
    Delay(250);
  end;

  Text:='Сделал Дима Башуров из Арзамаса-16 '+
        '(E-Mail: 0669@RFNC.NNOV.SU )';
  Print(Text,68,2,7,7,8,Video);

  Text:='Телефон в Арзамасе-16: (83130) 5-92-73';
  Print(Text,176,12,7,7,8,Video);

  Text:='Посвящается друзьям';
  Print(Text,80,27,0,13,25,Video);
  Print(Text,80,28,0,13,25,Video);

  FillEga($6040,70,80,0);

  Text:='СПРАВКА: Для перемещения своей руки использйте клавиши '+
        'со стрелками или';
  Print(Text,27,310,7,7,8,Video);

  Text:='"мышку". Ввод осуществляется нажатием на клавишу ПРОБЕЛ или на';
  Print(Text,99,320,7,7,8,Video);

  Text:='левую кнопку"мышки". Нажатие <Ctrl+S> включает/выключает звук,';
  Print(Text,99,330,7,7,8,Video);

  Text:='если пришел начальник, нажми клавишу TAB, ESC - выход из игры!';
  Print(Text,99,340,7,7,8,Video);

  WaitKey(0);
end;

procedure Footer;
var S:string;
begin
  S:='Автор Дима Башуров из Российского Федерального Ядерного Центра';
  Print(S,320-Length(S)*4,332,0,7,8,Video);

  S:='Телефон в Арзамасе-16 : (831-30) 5-92-73   '+
     'E-mail: 0669 @ RFNC. NNOV. SU';
  Print(S,320-Length(S)*4,341,0,7,8,Video);
end;

procedure BeruPriz;
var
  Rez,TryNo,Y,Unused:integer;
  Money:longint;
begin
  FillEga(0,350,80,7);

  PutPic(Pic[46]^,Video,10,10,7);
  PutPic(Pic[47]^,Video,200,10,7);
  PutPic(Pic[27]^,Video,480,172,7);
  PutPic(Pic[45]^,Video,511,173,16);
  PutPic(Pic[43]^,Video,532,209,16);

  TryNo:=0;

  repeat
    case TryNo of
      0:begin Text:='СТО'; Money:=10 end;
      1:begin Text:='ТЫЩА'; Money:=100 end;
      2:begin Text:='СТО ТЫЩ'; Money:=10000 end;
      3:begin Text:='МИЛЛИОН'; Money:=100000 end;
    end;

    Yak(17);
    Rez:=Choice(3);
    YakAnim;
    Inc(TryNo);
  until (TryNo>3) or (Rez=0);

  if Rez=0 then begin
    Yak(18);
    repeat
      Money:=Money-1;
      Sound(Random(50));
      PutPic(Pic[56]^,Video,Random(400),Random(295),2);
      NoSound;
    until Money<=0;
  end else begin
    Yak(20);

    Y:=190;

    Text:='Вы выбрали ПРИЗ и мы Вас поздравляем!';
    Y:=Y+18;
    Print(Text,240-Length(Text)*4,Y,0,13,8,Video);

    Text:='Фирма ИНТЕРМОДА и ПОЛЕ ЧУДЕС дарит Вам';
    Y:=Y+18;
    Print(Text,240-Length(Text)*4,Y,0,13,8,Video);

    Text:=Prizy[Random(10)]+' компании PROCTER & GAMBLE!';
    Y:=Y+18;
    Print(Text,240-Length(Text)*4,Y,15,13,8,Video);

    Text:='За ПРИЗОМ обращайтесь по адресу:';
    Y:=Y+18;
    Print(Text,240-Length(Text)*4,Y,0,13,8,Video);

    Text:='101000-Ц, Москва, проезд Серова, 11';
    Y:=Y+18;
    Print(Text,240-Length(Text)*4,Y,15,13,8,Video);

    Text:='На конверте сделайте пометку КОМПЬЮТЕРНЫЙ ПРИЗ';
    Y:=Y+18;
    Print(Text,240-Length(Text)*4,Y,0,13,8,Video);
  end;

  Footer;
  WaitKey(0);
  YakAnim;
end;

procedure Records;
var
  I,Y,Place:integer;
begin
  FillEga(0,350,80,7);

  PutPic(Pic[46]^,Video,10,10,7);
  PutPic(Pic[47]^,Video,200,10,7);
  PutPic(Pic[27]^,Video,480,172,7);
  PutPic(Pic[45]^,Video,511,173,16);
  PutPic(Pic[43]^,Video,532,209,16);

  Y:=190;

  Str(Ochki[3],Text);
  Text:='Вы выиграли в ФИНАЛЕ и набрали '+Text+' очков!';
  Y:=Y+18;
  Print(Text,240-Length(Text)*4,Y,0,13,8,Video);

  Text:='Торговый дом ТУСАР и ПОЛЕ ЧУДЕС дарит Вам';
  Y:=Y+18;
  Print(Text,240-Length(Text)*4,Y,0,13,8,Video);

  Text:=Prizy[Random(10)]+' компании PROCTER & GAMBLE!';
  Y:=Y+18;
  Print(Text,240-Length(Text)*4,Y,15,13,8,Video);

  Text:='За ПРИЗОМ обращайтесь по адресу:';
  Y:=Y+18;
  Print(Text,240-Length(Text)*4,Y,0,13,8,Video);

  Text:='101000-Ц, Москва, проезд Серова, 11';
  Y:=Y+18;
  Print(Text,240-Length(Text)*4,Y,15,13,8,Video);

  Text:='На конверте сделайте пометку КОМПЬЮТЕРНЫЙ ПРИЗ';
  Y:=Y+18;
  Print(Text,240-Length(Text)*4,Y,0,13,8,Video);

  Footer;

  Yak(13);
  WaitKey(0);
  YakAnim;

  Place:=0;

  if Ochki[3]>Rekordy[8].Score then begin
    Y:=8;
    while (Y>1) and (Ochki[3]>Rekordy[Y-1].Score) do Dec(Y);

    for I:=8 downto Y+1 do Rekordy[I]:=Rekordy[I-1];

    Rekordy[Y].Score:=Ochki[3];
    Rekordy[Y].Name:='';
    Place:=Y;
  end;

  FillEga($B25C,160,20,0);

  Text:='8 лучших игроков,';
  Print(Text,489,170,15,13,8,Buf);
  Print(Text,488,170,7,13,8,Buf);

  Text:='выигравших ФИНАЛ!';
  Print(Text,489,184,15,13,8,Buf);
  Print(Text,488,184,7,13,8,Buf);

  for I:=1 to 8 do with Rekordy[I] do begin
    if I=Place then Y:=15 else Y:=11;

    Str(I,Text);
    Text:=Text+' '+Name;
    Print(Text,481,I*14+195,Y-8,13,8,Buf);
    Print(Text,480,I*14+195,Y,13,8,Buf);

    Str(Score:4,Text);
    Text:=Text+'$';
    Print(Text,591,I*14+195,Y-8,13,8,Buf);
    Print(Text,590,I*14+195,Y,13,8,Buf);
  end;

  for I:=80 downto 0 do begin
    Sound((1600-I*20)*Zvuk);
    MoveVideo($B0CC,I*2*80+$33CC,160-I*2,19);
    NoSound;
    Delay(I);
  end;

  if Place>0 then begin
    EnterName(Place);
    Text:='pole.pic';
    Assign(FPic,Text);
    Rewrite(FPic);
    for I:=1 to 8 do Write(FPic,Rekordy[I]);
    Close(FPic);
  end;

  WaitKey(0);
end;

procedure Reklama;
var
  I,Y,Unused:integer;
  UnusedText:string;
begin
  MoveVideo($35FB,$B2FB,170,21);
  PutPic(Pic[52]^,Buf,481,173,16);

  Y:=164;

  Text:='Компьютерная игра';
  Y:=Y+10;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='продается по адресу';
  Y:=Y+74;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='101000-Ц, МОСКВА,';
  Y:=Y+8;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='проезд Серова, 11.';
  Y:=Y+8;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='25 самых первых';
  Y:=Y+8;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='покупателей будут';
  Y:=Y+8;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='приглашены со';
  Y:=Y+8;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='своими семьями';
  Y:=Y+8;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='на съемки телеигры';
  Y:=Y+8;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  Text:='ПОЛЕ ЧУДЕС!';
  Y:=Y+10;
  Print(Text,557-Length(Text)*4,Y,14,7,8,Buf);

  for I:=80 downto 0 do begin
    Sound((1700-I*20)*Zvuk);
    MoveVideo($B34B,I*2*80+$364B,160-I*2,21);
    NoSound;
    Delay(I div 2);
  end;

  WaitKey(0);

  PutPic(Pic[27]^,Video,480,172,7);
  PutPic(Pic[45]^,Video,511,173,16);
  PutPic(Pic[43]^,Video,532,209,16);
end;

procedure PlayersIntro;
var
  Phase,Beep,XB,Y,N,PersonNo:integer;
begin
  Yak(19);

  for N:=0 to 2 do begin
    if N<2 then PersonNo:=Sostav[Tur*2+1+N]
    else PersonNo:=0;

    case N of
      0:begin XB:=48; Y:=149 end;
      1:begin XB:=10; Y:=149 end;
      2:begin XB:=30; Y:=220 end;
    end;

    for Phase:=0 to 3 do begin
      FillEga(XB+$7D00+Y*80,90,10,7);

      if Phase<3 then
        PutPic(Pic[Phase+53]^,Buf,XB shl 3,Y,2)
      else if PersonNo>0 then
        PutPic(Pic[Persons[PersonNo].PicNo]^,Buf,XB shl 3,Y,2);

      if N<2 then
        MoveVideo(XB+$7D00+Y*80,Y*80+XB,90,10);

      for Beep:=1 to 20 do begin
        Sound((Beep*10+Phase*100+50)*Zvuk);
        Delay(1); NoSound;
        Delay(Phase shl 2+Beep div 4);
      end;
    end;

    if N=0 then Text:='Первый игрок';
    if N=1 then Text:='Второй игрок';
    if N=2 then Text:='Третий игрок';

    if PersonNo=0 then begin
      Print(Text,(XB+9) shl 3,Y+72,0,13,8,Video);
      Print('это ВЫ!',(XB+9) shl 3+24,Y+86,0,13,8,Video);
    end else begin
      Print(Text,XB shl 3,Y-29,0,13,8,Video);
      Print(Persons[PersonNo].Name,XB shl 3+4,Y-16,0,13,8,Video);
    end;
  end;

  WaitKey(0);
  YakAnim;
end;

begin
  Randomize;

  LoadFiles;
  InitVideo;

  Zvuk:=1;

  for I:=1 to 6 do Sostav[I]:=I;

  for K:=1 to 20 do begin
    I:=Random(6)+1;
    Tmp2:=Random(6)+1;
    Tmp:=Sostav[I];
    Sostav[I]:=Sostav[Tmp2];
    Sostav[Tmp2]:=Tmp;
  end;

  Intro;

  Tur:=0;

  for I:=1 to 3 do Ochki[I]:=0;
  for I:=0 to 2 do PrevSlova[I]:=0;

  repeat
    for I:=1 to 2 do
      Ochki[I]:=Tur*(Random(4)*15+5);

    Port[$3CE]:=5;
    Port[$3CF]:=0;

    SetFillStyle(1,3);
    Bar(0,0,640,10);

    SetFillStyle(1,8);
    Bar(0,10,640,12);

    SetFillStyle(1,1);
    Bar(0,13,640,107);

    SetFillStyle(1,8);
    Bar(0,108,640,110);

    SetFillStyle(1,7);
    Bar(0,111,640,350);

    SetFillStyle(1,8);
    Bar(0,330,640,331);

    for K:=0 to 35 do
      PutPic(Pic[Random(3)+31]^,Video,
             K mod 12*52+5,K div 12*31+15,1);

    PutPic(Pic[40]^,Video,0,25,7);
    PutPic(Pic[41]^,Video,600,25,7);
    PutPic(Pic[34]^,Video,69,3,1);
    PutPic(Pic[34]^,Video,559,3,1);

    Port[$3CE]:=5;
    Port[$3CF]:=0;

    SetFillStyle(1,7);
    Bar(120,15,520,95);

    SetFillStyle(1,8);
    SetColor(0);

    for I:=0 to 4 do
      Line(120,I*20+15,520,I*20+15);

    for I:=0 to 25 do
      Line(I shl 4+120,95,I shl 4+120,15);

    case Tur of
      0:Text:='ЧЕТВЕРТЬФИНАЛ';
      1:Text:='  ПОЛУФИНАЛ';
      2:Text:='    ФИНАЛ';
    end;

    Print(Text,204,78,0,13,16,Video);

    PutPic(Pic[27]^,Video,480,172,7);
    PutPic(Pic[45]^,Video,511,173,16);
    PutPic(Pic[43]^,Video,532,209,16);

    for K:=0 to 31 do
      PutPic(Pic[17]^,Video,K*20,332,8);

    for K:=0 to 31 do
      Print(Alfavit[K+1],K*20+4,334,0,13,8,Video);

    BarPos:=0;
    DrawBaraban;

    PlayersIntro;

    for I:=0 to 31 do Nazvano[I]:=0;
    for I:=1 to 3 do DrawScore(I);

    NewWord;

    SetFillStyle(1,8);

    for I:=1 to Length(Slovo) do Otkryto[I]:=0;

    for I:=1 to Length(Slovo) do
      Bar((I+5) shl 4+121,36,(I+5) shl 4+134,54);

    Yak(9);
    WaitKey(50);
    YakAnim;

    Yak(10);
    WaitKey(0);
    YakAnim;

    Igrok:=3;
    Vernyh:=0;

    repeat
      if Vernyh=3 then begin
        Yak(5);
        Boxes;
        Vernyh:=0;
      end;

      Sovpalo:=0;

      if Igrok>3 then Igrok:=1;

      Yak(1);
      Result:=0;

      if Igrok=3 then Result:=Choice(1);

      if Result=0 then begin
        Baraban;

        if Ochki[0]<>100 then Inc(Vernyh);

        Yak(2);

        if (Ochki[0]=300) and (Igrok=3) then begin
          Result:=Choice(2);
          if Result=0 then begin
            YakAnim;
            Yak(15);
          end;
        end;

        if Result=0 then begin
          WaitKey(40);
          YakAnim;

          if Ochki[0]<0 then begin
            Ochki[Igrok]:=0;
            DrawScore(Igrok);
          end;

          if Ochki[0]<=0 then begin
            Inc(Igrok);
            Vernyh:=0;
          end else begin
            if Ochki[0]=100 then PlusLetter
            else Letter;

            if Sovpalo>0 then GirlRun
            else begin
              Inc(Igrok);
              Vernyh:=0;
            end;
          end;

          Tmp2:=0;

          for I:=1 to Length(Slovo) do
            if Otkryto[I]=0 then Inc(Tmp2);
        end;
      end;

    until (Tmp2=0) or (Result>0);

    if Tmp2=0 then begin
      Yak(4);
      WaitKey(50);
      YakAnim;

      if Igrok=3 then Result:=0
      else Result:=1;
    end else begin
      YakAnim;

      if Result=2 then BeruPriz
      else if Result>0 then begin
        Result:=EnterWord;

        if Result=0 then Yak(11)
        else Yak(12);
      end;

      WaitKey(50);
      YakAnim;
    end;

    if Result=0 then begin
      Inc(Tur);

      Yak(16);
      WaitKey(0);
      YakAnim;

      Reklama;
    end;

  until (Result>0) or (Tur>2);

  if Tur>2 then Records
  else begin
    Yak(14);
    WaitKey(0);
    YakAnim;
  end;

  Quit;
end.