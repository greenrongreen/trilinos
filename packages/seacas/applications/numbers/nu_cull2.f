C    Copyright (c) 2014, Sandia Corporation.
C    Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
C    the U.S. Government retains certain rights in this software.
C    
C    Redistribution and use in source and binary forms, with or without
C    modification, are permitted provided that the following conditions are
C    met:
C    
C        * Redistributions of source code must retain the above copyright
C          notice, this list of conditions and the following disclaimer.
C    
C        * Redistributions in binary form must reproduce the above
C          copyright notice, this list of conditions and the following
C          disclaimer in the documentation and/or other materials provided
C          with the distribution.
C    
C        * Neither the name of Sandia Corporation nor the names of its
C          contributors may be used to endorse or promote products derived
C          from this software without specific prior written permission.
C    
C    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
C    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
C    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
C    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
C    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
C    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
C    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
C    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
C    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
C    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
C    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
C    

C $Id: cull2.f,v 1.1 1991/02/21 15:42:45 gdsjaar Exp $
C $Log: cull2.f,v $
C Revision 1.1  1991/02/21 15:42:45  gdsjaar
C Initial revision
C
      SUBROUTINE CULL2 (DIRCOS, MASSLV, NIQM)
      DIMENSION DIRCOS(4,*), MASSLV(2,*)
C
      J = 0
      DO 10 I=1, NIQM
          IF (MASSLV(2,I) .NE. 0) THEN
              J = J + 1
              MASSLV(1, J) = MASSLV(1, I)
              MASSLV(2, J) = MASSLV(2, I)
              DIRCOS(1, J) = DIRCOS(1, I)
              DIRCOS(2, J) = DIRCOS(2, I)
              DIRCOS(3, J) = DIRCOS(3, I)
              DIRCOS(4, J) = DIRCOS(4, I)
          END IF
   10 CONTINUE
C
      NIQM = J
      RETURN
      END
