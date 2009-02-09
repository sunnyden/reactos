@ stub SamAddMemberToAlias
@ stub SamAddMemberToGroup
@ stub SamAddMultipleMembersToAlias
@ stub SamChangePasswordUser
@ stub SamChangePasswordUser2
@ stub SamChangePasswordUser3
@ stub SamCloseHandle
@ stub SamConnect
@ stub SamConnectWithCreds
@ stub SamCreateAliasInDomain
@ stub SamCreateGroupInDomain
@ stub SamCreateUser2InDomain
@ stub SamCreateUserInDomain
@ stub SamDeleteAlias
@ stub SamDeleteGroup
@ stub SamDeleteUser
@ stub SamEnumerateAliasesInDomain
@ stub SamEnumerateDomainsInSamServer
@ stub SamEnumerateGroupsInDomain
@ stub SamEnumerateUsersInDomain
@ stub SamFreeMemory
@ stub SamGetAliasMembership
@ stub SamGetCompatibilityMode
@ stub SamGetDisplayEnumerationIndex
@ stub SamGetGroupsForUser
@ stub SamGetMembersInAlias
@ stub SamGetMembersInGroup
@ stub SamLookupDomainInSamServer
@ stub SamLookupIdsInDomain
@ stub SamLookupNamesInDomain
@ stub SamOpenAlias
@ stub SamOpenDomain
@ stub SamOpenGroup
@ stub SamOpenUser
@ stub SamQueryDisplayInformation
@ stub SamQueryInformationAlias
@ stub SamQueryInformationDomain
@ stub SamQueryInformationGroup
@ stub SamQueryInformationUser
@ stub SamQuerySecurityObject
@ stub SamRemoveMemberFromAlias
@ stub SamRemoveMemberFromForeignDomain
@ stub SamRemoveMemberFromGroup
@ stub SamRemoveMultipleMembersFromAlias
@ stub SamSetInformationDomain
@ stub SamSetInformationGroup
@ stub SamSetInformationUser
@ stub SamSetMemberAttributesOfGroup
@ stub SamSetSecurityObject
@ stub SamShutdownSamServer
@ stub SamTestPrivateFunctionsDomain
@ stub SamTestPrivateFunctionsUser
@ stub SamiChangeKeys
@ stub SamiChangePasswordUser
@ stub SamiChangePasswordUser2
@ stub SamiEncryptPasswords
@ stub SamiGetBootKeyInformation
@ stub SamiLmChangePasswordUser
@ stub SamiOemChangePasswordUser2
@ stub SamiSetBootKeyInformation
@ stub SamiSetDSRMPassword
@ stub SamiSetDSRMPasswordOWF

@ stdcall SamInitializeSAM()
@ stdcall SamGetDomainSid(ptr)
@ stdcall SamSetDomainSid(ptr)
@ stdcall SamCreateUser(wstr wstr ptr)
@ stdcall SamCheckUserPassword(wstr wstr)
@ stdcall SamGetUserSid(wstr ptr)
