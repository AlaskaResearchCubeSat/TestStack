// CrossWorks Tasking Library.
//
// Copyright (c) 2004, 2007, 2008, 2010 Rowley Associates Limited.
//
// This file may be distributed under the terms of the License Agreement
// provided with this software.
//
// THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

load("ctl/source/getState.js");

function getregs(x)
{
  var sp = Debug.evaluate("((CTL_TASK_t*)"+x+")->stack_pointer");
  var a = new Array();
  for (i=4;i<=15;i++) // r4-r15 saved
    {
      a[i] = TargetInterface.peekUint16(sp);
      sp += 2;
    }
  a[3] = 0; // r3 isn't saved  
  a[2] = TargetInterface.peekUint16(sp); // sr+pc
  a[0] = (a[2]&0xf000)<<4;
  a[2] &= 0x0fff;
  sp += 2;
  a[0] |= TargetInterface.peekUint16(sp); // pc
  sp += 2;
  a[1] = sp; // r1/sp
  return a;
}

function setregs(x, a)
{
  var sp = a[1];
  sp -= 2;
  TargetInterface.pokeUint16(sp, a[0]); // pc
  sp -= 2;
  var srpc = (a[2] & 0x0fff)+((a[0] >> 4) & 0xf000);
  TargetInterface.pokeUint16(sp, srpc); // sr+pc
  for (i=15;i>=4;i--)
    {
      sp -= 2;
      TargetInterface.pokeUint16(sp, a[i])
    }
  Debug.evaluate("((CTL_TASK_t*)"+x+")->stack_pointer="+sp);
}

function init()
{
  Threads.setColumns("Name", "Priority", "State", "Time");
  Threads.setSortByNumber("Time");
}

function update() 
{
  Threads.clear();
  var exe=Debug.evaluate("ctl_task_executing");
  var x=Debug.evaluate("ctl_task_list");
  if (x)
    Threads.newqueue("Task List");
  var count=0;
  while (x && count<10)
    {
      var xt = Debug.evaluate("*(CTL_TASK_t*)"+x);         
      if (x==exe)
        Threads.add(xt.name, xt.priority, "executing", xt.execution_time, []);
      else
        Threads.add(xt.name, xt.priority, getState(xt.state), xt.execution_time, x);
      x=xt.next;
      count++;
    }
}