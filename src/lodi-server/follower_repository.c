/**
* Provides persistence for registered client ip addresses and ports
 **/

#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "collections/list.h"
#include "collections/int_map.h"
#include "follower_repository.h"
#include "shared.h"

static IntMap *idolMap = NULL;
static IntMap *followerMap = NULL;

/**
 *  Constructor
 */
void initFollowerRepository() {
  createMap(&idolMap);
  createMap(&followerMap);
}

static int addFollowerIdol(unsigned int idolId, unsigned int followerId) {
  List *idols = NULL;
  int rt = followerMap->get(followerMap, followerId, (void **) &idols);
  if (rt == ERROR) {
    return ERROR;
  }
  if (rt == NOT_FOUND) {
    if (createList(&idols) != SUCCESS) {
      return ERROR;
    }
    followerMap->add(followerMap, followerId, idols);
  }
  for (int i = 0; i < idols->length; i++) {
    int *idol= NULL;
    idols->get(idols, i, (void **) &idol);
    if (*idol == idolId) {
      printf("Warning - idolId=%u already added to idol list for followerId=%u\n",
             idolId, followerId);
      return SUCCESS;
    }
  }
  int *persistedIdolId = malloc(sizeof(unsigned int));
  if (!persistedIdolId) {
    return ERROR;
  }
  *persistedIdolId = idolId;
  idols->append(idols, persistedIdolId);
  return SUCCESS;
}

static int addIdolFollower(unsigned int idolId, unsigned int followerId) {
  List *followers = NULL;
  int rt = idolMap->get(idolMap, idolId, (void **) &followers);
  if (rt == ERROR) {
    return ERROR;
  }
  if (rt == NOT_FOUND) {
    if (createList(&followers) != SUCCESS) {
      return ERROR;
    }
    idolMap->add(idolMap, idolId, followers);
  }
  for (int i = 0; i < followers->length; i++) {
    int *follower = NULL;
    followers->get(followers, i, (void **) &follower);
    if (*follower == followerId) {
      printf("Warning - followerId=%u already added to follower list for idolId=%u\n",
             followerId, idolId);
      return SUCCESS;
    }
  }
  int *persistedFollowerId = malloc(sizeof(unsigned int));
  if (!persistedFollowerId) {
    return ERROR;
  }
  *persistedFollowerId = followerId;
  followers->append(followers, persistedFollowerId);
  return SUCCESS;
}



int addFollower(unsigned int idolId, unsigned int followerId) {
  if (!idolMap) {
    return ERROR;
  }
  int ret;
  if ((ret = addIdolFollower(idolId, followerId) != SUCCESS)) {
    return ret;
  }
  return addFollowerIdol(idolId, followerId);
}

int removeFollower(unsigned int idolId, unsigned int followerId) {
  if (!idolMap) {
    return ERROR;
  }
  List *followers = NULL;
  int rt = idolMap->get(idolMap, idolId, (void **) &followers);
  if (rt == ERROR || rt == NOT_FOUND) {
    return rt;
  }
  for (int i = 0; i < followers->length; i++) {
    int *follower = NULL;
    followers->get(followers, i, (void **) &follower);
    if (*follower == followerId) {
      int * persistedFollowerId = NULL;
      followers->remove(followers, i, (void **) &persistedFollowerId);
      if (persistedFollowerId) {
        free(persistedFollowerId);
      } else {
        printf("Failed to retrieve id on removal, returning error...\n");
        return ERROR;
      }
      printf("Removed followerId=%u from list for idolId=%u\n",
             followerId, idolId);
      return SUCCESS;
    }
  }
  printf("Warning... followerId=%u not found for idolId=%u\n", followerId, idolId);
  return NOT_FOUND;
}


int getIdolFollowers(const unsigned int idolId, List **followers) {
  if (!idolMap) {
    return ERROR;
  }
  const int rt = idolMap->get(idolMap, idolId, (void **) followers);
  if (rt == ERROR || rt == NOT_FOUND) {
    return rt;
  }
  if ((*followers)->length == 0) {
    return NOT_FOUND;
  }
  return SUCCESS;
}

int getFollowerIdols(const unsigned int followerId, List **idols) {
  if (!followerMap) {
    return ERROR;
  }
  const int rt = followerMap->get(followerMap, followerId, (void **) idols);
  if (rt == ERROR || rt == NOT_FOUND) {
    return rt;
  }
  if ((*idols)->length == 0) {
    return NOT_FOUND;
  }
  return SUCCESS;
}