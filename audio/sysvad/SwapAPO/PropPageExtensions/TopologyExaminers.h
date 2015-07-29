//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    TopologyExaminers.h
//
// Abstract:    An interface and supporting class(es) for examining device
//              model IParts
//
// ----------------------------------------------------------------------------

#pragma once


// ----------------------------------------------------------------------
// interface IExaminer
// An interface for examining devicemodel IParts
// ----------------------------------------------------------------------
interface IExaminer
{
public:
    virtual HRESULT Examine(IPartsList* pPath) = 0;
    virtual BOOL    IsSatisfied() = 0;
};


// --------------------------------------------------------------------------------
// class CFormatExaminer
// A class that examines IConnectors looking for a soft connector that supports
// a given KS streaming format (KSDATAFORMAT).
// ----------------------------------------------------------------------
class CFormatExaminer : public IExaminer
{
private:
    PKSDATAFORMAT   m_pFormatReq;
    BOOL            m_bSatisfied;

    HRESULT ExaminePart(IPart* pIPart);

public:
    CFormatExaminer(PKSDATAFORMAT pKsFormat, ULONG cbFormat);
    ~CFormatExaminer();

    HRESULT Examine(IPartsList* pPath);
    BOOL    IsSatisfied()       {   return m_bSatisfied;    }
};
