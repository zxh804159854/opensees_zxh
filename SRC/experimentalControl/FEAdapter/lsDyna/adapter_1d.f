** ****************************************************************** **
**    OpenFRESCO - Open Framework                                     **
**                 for Experimental Setup and Control                 **
**                                                                    **
**                                                                    **
** Copyright (c) 2006, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited. See    **
** file 'COPYRIGHT_UCB' in main directory for information on usage    **
** and redistribution, and for a DISCLAIMER OF ALL WARRANTIES.        **
**                                                                    **
** Developed by:                                                      **
**   Andreas Schellenberg (andreas.schellenberg@gmx.net)              **
**   Yoshikazu Takahashi (yos@catfish.dpri.kyoto-u.ac.jp)             **
**   Gregory L. Fenves (fenves@berkeley.edu)                          **
**   Stephen A. Mahin (mahin@berkeley.edu)                            **
**                                                                    **
** ****************************************************************** **

** ****************************************************************** **
**  LS-DYNA (R) is a registered trademark of                          **
**  Livermore Software Technology Corporation in the United States.   **
**                                                                    **
** ****************************************************************** **

** $Revision$
** $Date$
** $Source: $

** Written: Yuli Huang (yulee@berkeley.edu)
** Created: 10/07
** Revision: A

** Description: This file contains the implementation of e101.
** e101 is a LS-DYNA (R) adapter element. The element communicates with the
** OpenFresco SimFEAdapter experimental control through a tcp/ip connection.
c
#include "define.inc"
      subroutine ushl_e101(force,stiff,ndtot,istif,
     *     x1,x2,x3,x4,y1,y2,y3,y4,z1,z2,z3,z4,
     *     fx1,fx2,fx3,fx4,
     *     fy1,fy2,fy3,fy4,
     *     fz1,fz2,fz3,fz4,
     *     xdof,
     *     dx1,dx2,dx3,dx4,dy1,dy2,dy3,dy4,dz1,dz2,dz3,dz4,
     *     wx1,wx2,wx3,wx4,wy1,wy2,wy3,wy4,wz1,wz2,wz3,wz4,
     *     dxdof,
     *     thick,thck1,thck2,thck3,thck4,
     *     hsv,nhsv,
     *     cm,lmc,
     *     gl11,gl21,gl31,gl12,gl22,gl32,gl13,gl23,gl33,
     *     cmtrx,lft,llt)
      
      include 'nlqparm'
      common/bk28/summss,xke,xpe,tt,xte0,erodeke,erodeie
      dimension force(nlq,ndtot),stiff(nlq,ndtot,ndtot)
      dimension x1(nlq),x2(nlq),x3(nlq),x4(nlq)
      dimension y1(nlq),y2(nlq),y3(nlq),y4(nlq)
      dimension z1(nlq),z2(nlq),z3(nlq),z4(nlq)
      dimension fx1(nlq),fx2(nlq),fx3(nlq),fx4(nlq)
      dimension fy1(nlq),fy2(nlq),fy3(nlq),fy4(nlq)
      dimension fz1(nlq),fz2(nlq),fz3(nlq),fz4(nlq)
      dimension xdof(nlq,8,3)
      dimension dx1(nlq),dx2(nlq),dx3(nlq),dx4(nlq)
      dimension dy1(nlq),dy2(nlq),dy3(nlq),dy4(nlq)
      dimension dz1(nlq),dz2(nlq),dz3(nlq),dz4(nlq)
      dimension wx1(nlq),wx2(nlq),wx3(nlq),wx4(nlq)
      dimension wy1(nlq),wy2(nlq),wy3(nlq),wy4(nlq)
      dimension wz1(nlq),wz2(nlq),wz3(nlq),wz4(nlq)
      dimension dxdof(nlq,8,3)
      dimension thick(nlq),thck1(nlq),thck2(nlq),thck3(nlq),thck4(nlq)
      dimension hsv(nlq,nhsv),cm(lmc)
      dimension gl11(nlq),gl21(nlq),gl31(nlq),
     *     gl12(nlq),gl22(nlq),gl32(nlq),
     *     gl13(nlq),gl23(nlq),gl33(nlq)
      dimension cmtrx(nlq,15,3)
      dimension x(2),y(2),z(2),dx(2),dy(2),dz(2),cs(3)
      
      integer*4 sizeData
      parameter (sizeData=256)
      
      integer*4 sizeInt, sizeDouble
      parameter (sizeInt=4, sizeDouble=8)
      
      integer*4 port
      integer*4 socketID
      integer*4 stat
      
      integer*4 iData(11)
      real*8    cData(sizeData)
      real*8    dData(sizeData)
      
      save cData, dData
      
      do i = lft, llt
         if (nint(hsv(i,1)) .eq. 0) then
            port = nint(cm(1))
            write(*,*) 'Waiting for ECSimAdapter experimental',
     *                 ' control on port', port
            call setupconnectionserver(port,socketID)
            if (socketID .le. 0) then
               write(*,*) 'ERROR - failed to setup connection'
               call adios(2)
            endif
            hsv(i,1) = socketID
c
c ...       get the data sizes
c
            call recvdata(socketID, sizeInt, iData, 11, stat)
            if (     iData(1) .gt. 1
     *          .or. iData(4) .gt. 1
     *          .or. iData(6) .gt. 1
     *          .or. iData(9) .gt. 1)
     *          then
               write(*,*) 'ERROR - wrong data sizes received != 1',
               call closeconnection(socketID, stat)
               call adios(2)
            endif
            write(*,*) 'Adapter element now running...'
         else
            socketID = nint(hsv(i,1))
         endif
c
c ...    update response if time has advanced
c
         if (tt .gt. cData(1)) then
c
c ...       receive data
c
            call recvdata(socketID, sizeDouble, cData, sizeData, stat)
c
c ...       check if force request was received
c
            if (cData(1) .eq. 10.) then
c
c ...          send daq displacements and forces
c
               call senddata(socketID, sizeDouble,
     *                       dData, sizeData, stat)
c
c ...          receive new trial response
c
               call recvdata(socketID, sizeDouble,
     *                       cData, sizeData, stat)
            endif
            
            if (cData(1) .ne. 3.) then
               if (cData(1) .eq. 99.) then
                  write(*,*) 'The Simulation has successfully completed'
                  call closeconnection(socketID, stat)
                  call adios(1)
               else
                  write(*,*) 'ERROR - Wrong action received'
                  call closeconnection(socketID, stat)
                  call adios(2)
               endif
            endif
c
c ...       save current time
c
            cData(1) = tt
         endif
c
c ...    set resisting force
c
         hsv(i,2) = hsv(i,2) + dx1(i)
         force(i,1) = (hsv(i,2) - cData(2))*cm(2)
c
c ...    assign daq values for feedback
c
         dData(1) = hsv(i,2)
         dData(2) = -force(i,1)
c
c ...    set stiffness
c
         if (istif .eq. 1) then
            stiff(i,1,1) = cm(2)
         endif
      enddo
      
      return 
      end
