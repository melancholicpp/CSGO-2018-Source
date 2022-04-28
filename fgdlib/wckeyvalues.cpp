//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ====
//
// Purpose: 
//
//=============================================================================

#include "fgdlib/wckeyvalues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
MDkeyvalue::~MDkeyvalue(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Assignment operator.
//-----------------------------------------------------------------------------
MDkeyvalue &MDkeyvalue::operator =(const MDkeyvalue &other)
{
	strcpy(szKey, other.szKey);
	strcpy(szValue, other.szValue);

	return(*this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void WCKVBase_Vector::RemoveKeyAt(int nIndex)
{
	Assert(nIndex >= 0);
	Assert(nIndex < (int)this->m_KeyValues.Count());

	if ((nIndex >= 0) && (nIndex < (int)this->m_KeyValues.Count()))
	{
		this->m_KeyValues.Remove(nIndex);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds the key to the keyvalue array. Allows duplicate keys.
//
//			NOTE: This should only be used for keyvalue lists that do not require
//			unique key names! If you use this function then you should use GetCount
//			and GetKey/Value by index rather than GetValue by key name.
//-----------------------------------------------------------------------------
void WCKVBase_Vector::AddKeyValue(const char *pszKey, const char *pszValue)
{
	if (!pszKey || !pszValue)
	{
		return;
	}

	char szTmpKey[KEYVALUE_MAX_KEY_LENGTH];
	char szTmpValue[KEYVALUE_MAX_VALUE_LENGTH];

	strcpy(szTmpKey, pszKey);
	strcpy(szTmpValue, pszValue);

	StripEdgeWhiteSpace(szTmpKey);
	StripEdgeWhiteSpace(szTmpValue);

	//
	// Add the keyvalue to our list.
	//
	MDkeyvalue newkv;
	strcpy(newkv.szKey, szTmpKey);
	strcpy(newkv.szValue, szTmpValue);
	this->m_KeyValues.AddToTail(newkv);
}

int WCKVBase_Vector::FindByKeyName( const char *pKeyName ) const
{
	for ( int i=0; i < this->m_KeyValues.Count(); i++ )
	{
		if ( V_stricmp( this->m_KeyValues[i].szKey, pKeyName ) == 0 )
			return i;
	}
	return GetInvalidIndex();
}

void WCKVBase_Vector::InsertKeyValue( const MDkeyvalue &kv )
{
	this->m_KeyValues.AddToTail( kv );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void WCKVBase_Dict::RemoveKeyAt(int nIndex)
{
	this->m_KeyValues.RemoveAt(nIndex);
}


int WCKVBase_Dict::FindByKeyName( const char *pKeyName ) const
{
	return this->m_KeyValues.Find( pKeyName );
}

void WCKVBase_Dict::InsertKeyValue( const MDkeyvalue &kv )
{
	this->m_KeyValues.Insert( kv.szKey, kv );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor. Sets the initial size of the keyvalue array.
//-----------------------------------------------------------------------------
template<class Base>
WCKeyValuesT<Base>::WCKeyValuesT(void)
{
}


//-----------------------------------------------------------------------------
// Purpose: Destructor. Deletes the contents of this keyvalue array.
//-----------------------------------------------------------------------------
template<class Base>
WCKeyValuesT<Base>::~WCKeyValuesT(void)
{
	//int i = 0;
	//while (i < this->m_KeyValues.GetSize())
	//{
	//	delete this->m_KeyValues.GetAt(i++);
	//}

	RemoveAll();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Base>
const char *WCKeyValuesT<Base>::GetValue(const char *pszKey, int *piIndex) const
{
	int i = this->FindByKeyName( pszKey );
	if ( i == this->GetInvalidIndex() )
	{
		return NULL;
	}
	else
	{
		if(piIndex)
			piIndex[0] = i;
			
		return this->m_KeyValues[i].szValue;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Base>
void WCKeyValuesT<Base>::RemoveKey(const char *pszKey)
{
	SetValue(pszKey, (const char *)NULL);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Base>
void WCKeyValuesT<Base>::SetValue(const char *pszKey, int iValue)
{
	char szValue[100];
	Q_snprintf(szValue, sizeof(szValue), "%d", iValue);

	SetValue(pszKey, szValue);
}


//-----------------------------------------------------------------------------
// Purpose: Strips leading and trailing whitespace from the string.
// Input  : psz - 
//-----------------------------------------------------------------------------
void StripEdgeWhiteSpace(char *psz)
{
	if (!psz || !*psz)
		return;

	char *pszBase = psz;

	while (V_isspace(*psz))
	{
		psz++;
	}

	int iLen = strlen(psz) - 1;
	
	if ( iLen >= 0 )
	{
		while (V_isspace(psz[iLen]))
		{
			psz[iLen--] = 0;
		}
	}

	if (psz != pszBase)
	{
		memmove(pszBase, psz, iLen + 2);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pszKey - 
//			pszValue - 
//-----------------------------------------------------------------------------
template<class Base>
void WCKeyValuesT<Base>::SetValue(const char *pszKey, const char *pszValue)
{
	char szTmpKey[KEYVALUE_MAX_KEY_LENGTH];
	char szTmpValue[KEYVALUE_MAX_VALUE_LENGTH];

	strcpy(szTmpKey, pszKey);

	if (pszValue != NULL)
	{
		strcpy(szTmpValue, pszValue);
	}
	else
	{
		szTmpValue[0] = 0;
	}

	StripEdgeWhiteSpace(szTmpKey);
	StripEdgeWhiteSpace(szTmpValue);

	int i = this->FindByKeyName( szTmpKey );
	if ( i == this->GetInvalidIndex() )
	{
		if ( pszValue )
		{
			//
			// Add the keyvalue to our list.
			//
			MDkeyvalue newkv;
			Q_strncpy( newkv.szKey, szTmpKey, sizeof( newkv.szKey ) );
			Q_strncpy( newkv.szValue, szTmpValue, sizeof( newkv.szValue ) );
			this->InsertKeyValue( newkv );
		}
	}
	else
	{
		if (pszValue != NULL)
		{
			V_strncpy(this->m_KeyValues[i].szValue, szTmpValue, sizeof(this->m_KeyValues[i].szValue));
		}
		//
		// If we are setting to a NULL value, delete the key.
		//
		else
		{
			this->RemoveKeyAt( i );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<class Base>
void WCKeyValuesT<Base>::RemoveAll(void)
{
	this->m_KeyValues.RemoveAll();
}


// Explicit instantiations.
template class WCKeyValuesT<WCKVBase_Dict>;
template class WCKeyValuesT<WCKVBase_Vector>;



