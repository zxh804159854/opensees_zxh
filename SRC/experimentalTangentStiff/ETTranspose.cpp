/* ****************************************************************** **
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
** ****************************************************************** */

// Written: Hong Kim (hongkim@berkeley.edu)
// Created: 10/10
// Revision: A
//
// Description: This file contains the class definition for 
// ETBfgs.  This class uses the measured displacement vector and 
// resisting for vector to compute the new stiffness matrix.  It uses 
// the BFGS method used by Thewalt and later by Igarashi 
// (Igarashi 1993 p. 10)

#include "ETTranspose.h"

#include <elementAPI.h>

void* OPF_ETTranspose()
{
    // pointer to experimental tangent stiff that will be returned
    ExperimentalTangentStiff* theTangentStiff = 0;
    
    if (OPS_GetNumRemainingInputArgs() < 2) {
        opserr << "WARNING invalid number of arguments\n";
        opserr << "Want: expTangentStiff Transpose tag numCols\n";
        return 0;
    }
    
    // tangent stiff tag
    int tag;
    int numdata = 1;
    if (OPS_GetIntInput(&numdata, &tag) != 0) {
        opserr << "WARNING invalid expTangentStiff Transpose tag\n";
        return 0;
    }
    
    // number of columns
    int numCols;
    numdata = 1;
    if (OPS_GetIntInput(&numdata, &numCols) != 0) {
        opserr << "WARNING invalid number of columns value\n";
        opserr << "expTangentStiff Transpose " << tag << endln;
        return 0;
    }
    
    // parsing was successful, allocate the tangent stiff
    theTangentStiff = new ETTranspose(tag, numCols);
    if (theTangentStiff == 0) {
        opserr << "WARNING could not create experimental tangent stiffness "
            << "of type ETTranspose\n";
        return 0;
    }
    
    return theTangentStiff;
}


ETTranspose::ETTranspose(int tag , int nC)
    : ExperimentalTangentStiff(tag), numCol(nC),
    theStiff(0), iDMatrix(0,0), iFMatrix(0,0)
{
    // does nothing
}


ETTranspose::ETTranspose(const ETTranspose& ets)
    : ExperimentalTangentStiff(ets), theStiff(0)
{
    numCol = ets.numCol;
}


ETTranspose::~ETTranspose()
{
    // invoke the destructor on any objects created by the object
    // that the object still holds a pointer to
    if (theStiff != 0)
        delete theStiff;
}


Matrix& ETTranspose::updateTangentStiff(
    const Vector* incrDisp,
    const Vector* incrVel,
    const Vector* incrAccel,
    const Vector* incrForce,
    const Vector* time,
    const Matrix* kInit,
    const Matrix* kPrev)
{
    // using incremental disp and force
    int dimR = kPrev->noRows();
    int dimC = kPrev->noCols();
    int szD	 = incrDisp->Size();
    int dimCiDM = iDMatrix.noCols();
    int i, j;
    
    Matrix tempDM(dimR, dimCiDM);
    Matrix tempFM(dimR, dimCiDM);
    Matrix iDMatrixT, iFMatrixT, tempDDT(dimC, dimR), tempDFT(dimC, dimR), theStiffT(dimC, dimR);
    // FIX ME add size check
    theStiff = new Matrix(dimR, dimC);
    theStiff->Zero();
    theStiffT.Zero();
    
    // append the incremental vectors to matrices
    if (dimCiDM < numCol) {
        // copy all entries from the existing matrices
        for (i=0; i<dimR; i++) {
            for (j=0; j<dimCiDM; j++) {
                tempDM(i,j) = iDMatrix(i,j);
                tempFM(i,j) = iFMatrix(i,j);
            }
        }
        iDMatrix.resize(dimR, dimCiDM +1);
        iFMatrix.resize(dimR, dimCiDM +1);
        for (i=0; i<dimR; i++) {
            for (j=0; j<dimCiDM; j++) {
                iDMatrix(i,j) = tempDM(i,j);
                iFMatrix(i,j) = tempFM(i,j);
            }
        }
        for (i=0; i<dimC; i++) {
            iDMatrix(i, dimCiDM) = (*incrDisp)(i);
            iFMatrix(i, dimCiDM) = (*incrForce)(i);
        }
    } else if (dimCiDM == numCol) {
        for (i=0; i<dimR; i++) {
            for (j=0; j<(dimCiDM-1); j++) {
                iDMatrix(i,j) = iDMatrix(i,j+1);
                iFMatrix(i,j) = iFMatrix(i,j+1);
            }
        }
        for (i=0; i<dimR; i++) {
            iDMatrix(i, dimCiDM-1) = (*incrDisp)(i);
            iFMatrix(i, dimCiDM-1) = (*incrForce)(i);
        }
    }
    this->MatTranspose(&iDMatrixT, &iDMatrix);
    this->MatTranspose(&iFMatrixT, &iFMatrix);
    
    // check how many columns are in iDMatrix
    if ((dimCiDM+1) < dimC) {
        theStiff->addMatrix(0.0, (*kInit), 1.0);
    } else if ((dimCiDM+1) == dimC) {
        iDMatrixT.Solve(iFMatrixT, theStiffT);
        this->MatTranspose(theStiff, &theStiffT);
    } else {
        tempDDT.addMatrixProduct(0.0, iDMatrix, iDMatrixT, 1.0);
        tempDFT.addMatrixProduct(0.0, iDMatrix, iFMatrixT, 1.0);
        tempDDT.Solve(tempDFT, theStiffT);
        this->MatTranspose(theStiff, &theStiffT);
    }
    
    return *theStiff;
}


ExperimentalTangentStiff* ETTranspose::getCopy()
{
    return new ETTranspose(*this);
}


void ETTranspose::Print(OPS_Stream &s, int flag)
{
    s << "Experimental Tangent: " << this->getTag(); 
    s << "  type: ETTranspose\n";
    s << "  numCol: " << numCol << endln;
}


Response* ETTranspose::setResponse(const char **argv,
    int argc, OPS_Stream &output)
{
    Response *theResponse = 0;
    
    output.tag("ExpTangentStiffOutput");
    output.attr("tangStifType",this->getClassType());
    output.attr("tangStifTag",this->getTag());
    
    // stiffness
    if (strcmp(argv[0],"stif") == 0 ||
        strcmp(argv[0],"stiff") == 0 ||
        strcmp(argv[0],"stiffness") == 0)
    {
        output.tag("ResponseType","tangStif");
        theResponse = new ExpTangentStiffResponse(this, 1, Matrix(1,1));
    }
    
    return theResponse;
}


int ETTranspose::getResponse(int responseID,
    Information &info)
{
    switch (responseID)  {
    case 1:  // stiffness
        if (theStiff != 0)
            return info.setMatrix(*theStiff);
        
    default:
        return OF_ReturnType_failed;
    }
}


int ETTranspose::MatTranspose(Matrix* kT, const Matrix* k)
{
    int dimR = k->noRows();
    int dimC = k->noCols();
    kT->resize(dimC,dimR);
    
    int i, j;
    //transpose matrix
    for (i=0; i<dimC; i++) {
        for (j=0; j<dimR; j++) {
            (*kT)(i,j) = (*k)(j,i);
        }
    }
    
    return OF_ReturnType_completed;
}
